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

std::optional<LUID> resolveLuidFromNameAndBus(const wstring& name, uint32_t busIndex) {
    ComPtr<IDXGIFactory1> factory;
    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return std::nullopt;

    for (UINT i = 0;; ++i) {
        ComPtr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(i, &adapter) == DXGI_ERROR_NOT_FOUND) break;

        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
        if (name.length() && wcsstr(desc.Description, name.c_str())) {
            // Match name, now verify PCI bus index
            HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
            if (devInfo == INVALID_HANDLE_VALUE) continue;

            SP_DEVINFO_DATA devData = {};
            devData.cbSize = sizeof(devData);

            for (DWORD j = 0; SetupDiEnumDeviceInfo(devInfo, j, &devData); ++j) {
                WCHAR location[256];
                DWORD type;
                ULONG size;

                if (!SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_LOCATION_INFORMATION,
                    &type, (PBYTE)location, sizeof(location), &size)) continue;

                wstring loc = location;
                size_t pos = loc.find(L"PCI bus ");
                if (pos == wstring::npos) continue;

                uint32_t parsedBus = wcstoul(loc.substr(pos + 8).c_str(), nullptr, 10);
                if (parsedBus == busIndex) {
                    SetupDiDestroyDeviceInfoList(devInfo);
                    return desc.AdapterLuid;
                }
            }
            SetupDiDestroyDeviceInfoList(devInfo);
        }
    }
    return std::nullopt;
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
        size_t comma = adapterSpec.find(L',');
        if (comma != wstring::npos) {
            wstring name = adapterSpec.substr(0, comma);
            uint32_t bus = wcstoul(adapterSpec.substr(comma + 1).c_str(), nullptr, 10);
            auto luidOpt = resolveLuidFromNameAndBus(name, bus);
            if (luidOpt.has_value()) {
                adapterLuid = luidOpt.value();
                hasTargetAdapter = true;
                return true;
            }
        } else {
            auto gpus = getAvailableGPUs();
            for (const auto& gpu : gpus) {
                if (_wcsicmp(gpu.name.c_str(), adapterSpec.c_str()) == 0) {
                    adapterLuid = gpu.desc.AdapterLuid;
                    hasTargetAdapter = true;
                    return true;
                }
            }
        }
        hasTargetAdapter = false;
        return false;
    }
};
