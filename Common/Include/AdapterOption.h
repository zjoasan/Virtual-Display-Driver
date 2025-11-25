#pragma once

// Core
#include <wrl/client.h>
#include <dxgi1_6.h>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <fstream>
#include <regex>

#include <setupapi.h>
#include <devguid.h>
#include <cfgmgr32.h>
#include <devpkey_shim.h>

#pragma comment(lib, "setupapi.lib")

using namespace std;
using namespace Microsoft::WRL;

struct GPUInfo {
    wstring name;                
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC desc;      
};

static inline vector<GPUInfo> getAvailableGPUs() {
    vector<GPUInfo> gpus;
    ComPtr<IDXGIFactory1> factory;
    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return gpus;
    }
    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter> adapter;
        if (!SUCCEEDED(factory->EnumAdapters(i, &adapter))) {
            break;
        }
        DXGI_ADAPTER_DESC desc{};
        if (FAILED(adapter->GetDesc(&desc))) continue;
        gpus.push_back({ desc.Description, adapter, desc });
    }
    return gpus;
}
struct PciLoc {
    uint32_t bus = UINT32_MAX;
    uint32_t device = UINT32_MAX;
    uint32_t function = UINT32_MAX;
    bool valid() const { return bus != UINT32_MAX && device != UINT32_MAX && function != UINT32_MAX; }
};

static inline bool parsePciTokenHex(const std::wstring& token, PciLoc& out) {
    auto l = token.find(L"PCI(");
    if (l == std::wstring::npos) return false;
    auto r = token.find(L')', l + 4);
    if (r == std::wstring::npos) return false;
    auto hexStr = token.substr(l + 4, r - (l + 4));
    unsigned value = 0;
    if (swscanf_s(hexStr.c_str(), L"%x", &value) != 1) return false;

    if (hexStr.size() >= 4) {
        out.bus      = (value >> 8) & 0xFF; // high byte
        out.device   = (value >> 4) & 0xF;  // next nibble
        out.function =  value       & 0xF;  // lowest nibble
        return true;
    } else if (hexStr.size() == 3) {
        out.bus      = (value >> 8) & 0xF;
        out.device   = (value >> 4) & 0xF;
        out.function =  value       & 0xF;
        return true;
    } else if (hexStr.size() == 2) {
        out.bus      = (value >> 4) & 0xF;
        out.device   =  value       & 0xF;
        out.function = 0;
        return true;
    }
    return false;
}

static inline PciLoc extractPciFromLocationPath(const std::wstring& path) {
    PciLoc loc{};
    size_t pos = path.rfind(L"PCI(");
    if (pos != std::wstring::npos) {
        parsePciTokenHex(path.substr(pos), loc);
    }
    return loc;
}

static inline PciLoc extractPciFromLocationInfo(const std::wstring& info) {
    PciLoc loc{};
    if (info.empty()) return loc;
    std::wregex re(L".*bus\\s*(\\d+).*device\\s*(\\d+).*function\\s*(\\d+)", std::regex_constants::icase);
    std::wsmatch m;
    if (std::regex_search(info, m, re) && m.size() >= 4) {
        loc.bus      = std::wcstoul(m[1].str().c_str(), nullptr, 10);
        loc.device   = std::wcstoul(m[2].str().c_str(), nullptr, 10);
        loc.function = std::wcstoul(m[3].str().c_str(), nullptr, 10);
    }
    return loc;
}

static inline std::vector<std::wstring> getLocationPaths(HDEVINFO h, SP_DEVINFO_DATA& devInfo) {
    std::vector<std::wstring> out;

    DEVPROPTYPE type = 0;
    DWORD size = 0;
    SetupDiGetDevicePropertyW(h, &devInfo, &DEVPKEY_Device_LocationPaths, &type,
                              nullptr, 0, &size, 0);
    if (size == 0 || type != DEVPROP_TYPE_STRING_LIST) return out;

    std::vector<BYTE> buf(size);
    if (!SetupDiGetDevicePropertyW(h, &devInfo, &DEVPKEY_Device_LocationPaths, &type,
                                   buf.data(), static_cast<DWORD>(buf.size()), &size, 0) ||
        type != DEVPROP_TYPE_STRING_LIST) {
        return out;
    }

    const wchar_t* p = reinterpret_cast<const wchar_t*>(buf.data());
    const wchar_t* end = reinterpret_cast<const wchar_t*>(buf.data() + size);
    while (p < end && *p) {
        std::wstring s = p;
        out.push_back(s);
        p += s.size() + 1;
    }
    return out;
}

static inline std::wstring getLocationInfo(HDEVINFO h, SP_DEVINFO_DATA& devInfo) {
    DEVPROPTYPE type = 0;
    DWORD size = 0;
    SetupDiGetDevicePropertyW(h, &devInfo, &DEVPKEY_Device_LocationInfo, &type,
                              nullptr, 0, &size, 0);
    if (size && type == DEVPROP_TYPE_STRING) {
        std::vector<wchar_t> buf(size / sizeof(wchar_t));
        if (SetupDiGetDevicePropertyW(h, &devInfo, &DEVPKEY_Device_LocationInfo, &type,
                                      reinterpret_cast<PBYTE>(buf.data()), size, &size, 0)) {
            return std::wstring(buf.data());
        }
    }

    WCHAR loc[256]{};
    if (SetupDiGetDeviceRegistryPropertyW(h, &devInfo, SPDRP_LOCATION_INFORMATION, nullptr,
                                          reinterpret_cast<PBYTE>(loc), sizeof(loc), nullptr)) {
        return loc;
    }
    return L"";
}


struct DxgiNode {
    DXGI_ADAPTER_DESC3 d3{};
    ComPtr<IDXGIAdapter4> ad4;
    UINT enumIndex{};
};

static inline std::vector<DxgiNode> enumDxgiNodes() {
    std::vector<DxgiNode> out;
    ComPtr<IDXGIFactory6> f;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&f)))) return out;

    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter4> a;
        auto hr = f->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&a));
        if (hr == DXGI_ERROR_NOT_FOUND) break;
        if (FAILED(hr)) continue;

        DXGI_ADAPTER_DESC3 d{};
        if (FAILED(a->GetDesc3(&d))) continue;
        if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        out.push_back({ d, a, i });
    }
    return out;
}

struct DevNode {
    std::wstring instanceId;
    std::wstring friendly;
    PciLoc pci{};
};

static inline std::vector<DevNode> enumDisplayDevNodesStable() {
    std::vector<DevNode> out;
    HDEVINFO h = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
    if (h == INVALID_HANDLE_VALUE) return out;

    SP_DEVINFO_DATA info{};
    info.cbSize = sizeof(info);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(h, i, &info); ++i) {
        DevNode dn{};

        WCHAR inst[512]{};
        if (SetupDiGetDeviceInstanceIdW(h, &info, inst, ARRAYSIZE(inst), nullptr))
            dn.instanceId = inst;

        WCHAR desc[256]{};
        if (SetupDiGetDeviceRegistryPropertyW(h, &info, SPDRP_DEVICEDESC, nullptr,
                                              (PBYTE)desc, sizeof(desc), nullptr))
            dn.friendly = desc;

        auto paths = getLocationPaths(h, info);
        if (!paths.empty()) {
            dn.pci = extractPciFromLocationPath(paths.front());
            if (!dn.pci.valid()) {
                for (const auto& p : paths) {
                    dn.pci = extractPciFromLocationPath(p);
                    if (dn.pci.valid()) break;
                }
            }
        }

        if (!dn.pci.valid()) {
            auto infoStr = getLocationInfo(h, info);
            dn.pci = extractPciFromLocationInfo(infoStr);
        }

        out.push_back(dn);
    }
    SetupDiDestroyDeviceInfoList(h);
    return out;
}

static inline bool nameContains(const std::wstring& hay, const std::wstring& needle) {
    return wcsstr(hay.c_str(), needle.c_str()) != nullptr;
}

static inline int pciTupleCmp(const PciLoc& a, const PciLoc& b) {
    if (a.bus != b.bus) return a.bus < b.bus ? -1 : 1;
    if (a.device != b.device) return a.device < b.device ? -1 : 1;
    if (a.function != b.function) return a.function < b.function ? -1 : 1;
    return 0;
}

static inline std::optional<LUID> resolveAdapter(const std::wstring& spec) {
    size_t comma = spec.find(L',');
    std::wstring name = spec;
    uint32_t requestedBus = UINT32_MAX;
    if (comma != std::wstring::npos) {
        name = spec.substr(0, comma);
        requestedBus = wcstoul(spec.substr(comma + 1).c_str(), nullptr, 10);
    }

    auto devs = enumDisplayDevNodesStable();
    auto dxgisAll = enumDxgiNodes();

    std::vector<DevNode> devMatches;
    for (const auto& d : devs) {
        if (nameContains(d.friendly, name)) {
            devMatches.push_back(d);
        }
    }
    if (devMatches.empty()) return std::nullopt;

    std::stable_sort(devMatches.begin(), devMatches.end(), [](const DevNode& a, const DevNode& b){
        const bool av = a.pci.valid(), bv = b.pci.valid();
        if (av != bv) return av; 
        if (!av && !bv) return a.instanceId < b.instanceId; 
        return pciTupleCmp(a.pci, b.pci) < 0;
    });

    const DevNode* chosenDev = &devMatches.front();
    if (requestedBus != UINT32_MAX) {
        auto exact = std::find_if(devMatches.begin(), devMatches.end(),
            [requestedBus](const DevNode& d){ return d.pci.valid() && d.pci.bus == requestedBus; });
        if (exact != devMatches.end()) {
            chosenDev = &*exact;
        } else {
            auto nearIt = std::min_element(devMatches.begin(), devMatches.end(),
                [requestedBus](const DevNode& a, const DevNode& b){
                    auto da = a.pci.valid() ? std::abs((int)a.pci.bus - (int)requestedBus) : INT_MAX/2;
                    auto db = b.pci.valid() ? std::abs((int)b.pci.bus - (int)requestedBus) : INT_MAX/2;
                    if (da != db) return da < db;
                    if (a.pci.valid() && b.pci.valid()) {
                        if (a.pci.device != b.pci.device) return a.pci.device < b.pci.device;
                        return a.pci.function < b.pci.function;
                    }
                    return a.instanceId < b.instanceId;
                });
            chosenDev = &*nearIt;
        }
    }

    std::vector<DxgiNode> dxgiMatches;
    for (auto& x : dxgisAll) {
        if (nameContains(x.d3.Description, name)) {
            dxgiMatches.push_back(x);
        }
    }
    if (dxgiMatches.empty()) return std::nullopt;

    std::sort(dxgiMatches.begin(), dxgiMatches.end(),
              [](const DxgiNode& a, const DxgiNode& b){ return a.enumIndex < b.enumIndex; });

    auto pos = std::distance(devMatches.begin(), std::find_if(devMatches.begin(), devMatches.end(),
        [chosenDev](const DevNode& d){ return d.instanceId == chosenDev->instanceId; }));

    if (pos < 0 || (size_t)pos >= dxgiMatches.size()) return std::nullopt;
    return dxgiMatches[(size_t)pos].d3.AdapterLuid;
}

class AdapterOption {
public:
    bool hasTargetAdapter{};
    LUID adapterLuid{};
    std::wstring target_name{};

    std::wstring selectBestGPU() {
        std::vector<DxgiNode> nodes = enumDxgiNodes();
        if (nodes.empty()) return L"";
        std::sort(nodes.begin(), nodes.end(), [](const DxgiNode& a, const DxgiNode& b){
            return a.d3.DedicatedVideoMemory > b.d3.DedicatedVideoMemory;
        });
        return nodes.front().d3.Description;
    }

    void load(const wchar_t* path) {
        std::ifstream ifs{ path };
        if (!ifs.is_open()) {
            target_name = selectBestGPU();
        } else {
            std::string line;
            getline(ifs, line);
            target_name.assign(line.begin(), line.end());
        }
        findAndSetAdapter(target_name);
    }

    void xmlprovide(const std::wstring& xtarg) {
        target_name = xtarg;
        findAndSetAdapter(target_name);
    }

    void apply(const IDDCX_ADAPTER& adapter) {
        if (hasTargetAdapter && IDD_IS_FUNCTION_AVAILABLE(IddCxAdapterSetRenderAdapter)) {
            IDARG_IN_ADAPTERSETRENDERADAPTER arg{};
            arg.PreferredRenderAdapter = adapterLuid;
            IddCxAdapterSetRenderAdapter(adapter, &arg);
        }
    }

private:
    bool findAndSetAdapter(const std::wstring& adapterSpec) {
        auto luidOpt = resolveAdapter(adapterSpec);
        if (luidOpt.has_value()) {
            adapterLuid = luidOpt.value();
            hasTargetAdapter = true;
            return true;
        }
        hasTargetAdapter = false;
        return false;
    }
};
