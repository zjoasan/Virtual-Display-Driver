#pragma once

#include <wrl/client.h>
#include <dxgi1_6.h>
#include <setupapi.h>   
#include <devguid.h>
#include <string>
#include <vector>
#include <optional>
#include <algorithm>
#include <fstream>

using namespace std;
using namespace Microsoft::WRL;

struct GPUInfo {
    wstring name;
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC desc;
};

bool CompareGPUs(const GPUInfo& a, const GPUInfo& b) {
    return a.desc.DedicatedVideoMemory > b.desc.DedicatedVideoMemory;
}

vector<GPUInfo> getAvailableGPUs() {
    vector<GPUInfo> gpus;
    ComPtr<IDXGIFactory1> factory;
    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return gpus;

    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter> adapter;
        if (!SUCCEEDED(factory->EnumAdapters(i, &adapter))) break;

        DXGI_ADAPTER_DESC desc;
        if (!SUCCEEDED(adapter->GetDesc(&desc))) continue;

        gpus.push_back({ desc.Description, adapter, desc });
    }
    return gpus;
}

struct DevNode {
    wstring instanceId;
    wstring friendly;
    uint32_t bus{ UINT32_MAX };
};

struct DxgiNode {
    DXGI_ADAPTER_DESC3 d3{};
    ComPtr<IDXGIAdapter4> ad4;
    UINT enumIndex{};
};

vector<DevNode> enumDisplayDevNodes() {
    vector<DevNode> out;
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
        SetupDiGetDeviceRegistryPropertyW(h, &info, SPDRP_DEVICEDESC, nullptr, (PBYTE)desc, sizeof(desc), nullptr);
        dn.friendly = desc;

        WCHAR loc[256]{};
        if (SetupDiGetDeviceRegistryPropertyW(h, &info, SPDRP_LOCATION_INFORMATION, nullptr, (PBYTE)loc, sizeof(loc), nullptr)) {
            wstring L = loc;
            auto pb = L.find(L"PCI bus ");
            if (pb != wstring::npos) {
                dn.bus = wcstoul(L.substr(pb + 8).c_str(), nullptr, 10);
            }
        }

        out.push_back(dn);
    }
    SetupDiDestroyDeviceInfoList(h);
    return out;
}


vector<DxgiNode> enumDxgiNodes() {
    vector<DxgiNode> out;
    ComPtr<IDXGIFactory6> f;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&f)))) return out;

    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter4> a;
        if (f->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_UNSPECIFIED, IID_PPV_ARGS(&a)) == DXGI_ERROR_NOT_FOUND) break;
        DXGI_ADAPTER_DESC3 d{};
        if (FAILED(a->GetDesc3(&d))) continue;
        if (d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        out.push_back({ d, a, i });
    }
    return out;
}

std::optional<LUID> resolveAdapter(const std::wstring& spec) {
    size_t comma = spec.find(L',');
    std::wstring name = spec;
    uint32_t busIndex = UINT32_MAX;
    if (comma != std::wstring::npos) {
        name = spec.substr(0, comma);
        busIndex = wcstoul(spec.substr(comma + 1).c_str(), nullptr, 10);
    }

    auto devs = enumDisplayDevNodes();
    auto dxgis = enumDxgiNodes();

    vector<DevNode> matches;
    for (auto& d : devs) {
        if (wcsstr(d.friendly.c_str(), name.c_str())) {
            matches.push_back(d);
        }
    }
    if (matches.empty()) return std::nullopt;

    if (matches.size() == 1) {
        for (auto& x : dxgis) {
            if (wcsstr(x.d3.Description, name.c_str())) {
                return x.d3.AdapterLuid;
            }
        }
        return std::nullopt;
    }

    const DevNode* chosenDev = &matches.front();
    if (busIndex != UINT32_MAX) {
        auto exact = std::find_if(matches.begin(), matches.end(), [busIndex](const DevNode& d){ return d.bus == busIndex; });
        if (exact != matches.end()) chosenDev = &*exact;
        else {
            auto nearIt = std::min_element(matches.begin(), matches.end(), [busIndex](const DevNode& a, const DevNode& b){
                return abs((int)a.bus - (int)busIndex) < abs((int)b.bus - (int)busIndex);
            });
            chosenDev = &*nearIt;
        }
    }

    sort(matches.begin(), matches.end(), [](const DevNode& a, const DevNode& b){ return a.bus < b.bus; });
    vector<DxgiNode> dxgiMatches;
    for (auto& x : dxgis) {
        if (wcsstr(x.d3.Description, name.c_str())) {
            dxgiMatches.push_back(x);
        }
    }
    sort(dxgiMatches.begin(), dxgiMatches.end(), [](const DxgiNode& a, const DxgiNode& b){ return a.enumIndex < b.enumIndex; });

    auto idx = distance(matches.begin(), find_if(matches.begin(), matches.end(),
        [chosenDev](const DevNode& d){ return d.instanceId == chosenDev->instanceId; }));

    if (idx < 0 || (size_t)idx >= dxgiMatches.size()) return std::nullopt;
    return dxgiMatches[idx].d3.AdapterLuid;
}

class AdapterOption {
public:
    bool hasTargetAdapter{};
    LUID adapterLuid{};
    wstring target_name{};

    wstring selectBestGPU() {
        auto gpus = getAvailableGPUs();
        if (gpus.empty()) return L"";
        sort(gpus.begin(), gpus.end(), CompareGPUs);
        return gpus.front().name;
    }

    void load(const wchar_t* path) {
        ifstream ifs{ path };
        if (!ifs.is_open()) {
            target_name = selectBestGPU();
        } else {
            string line;
            getline(ifs, line);
            target_name.assign(line.begin(), line.end());
        }
        findAndSetAdapter(target_name);
    }

    void xmlprovide(const wstring& xtarg) {
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
    bool findAndSetAdapter(const wstring& adapterSpec) {
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
