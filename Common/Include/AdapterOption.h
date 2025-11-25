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
#include <cwchar>
#include <iomanip>
#include <cctype> // Required for towlower in the case-insensitive search

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "setupapi.lib")

using namespace std;
using namespace Microsoft::WRL;

// DEVPKEY_Device_Luid: {60b193cb-5276-4d0f-96fc-f173ab17af69}, 2
static const DEVPROPKEY DEVPKEY_Device_Luid_Custom = {
    { 0x60b193cb, 0x5276, 0x4d0f, { 0x96, 0xfc, 0xf1, 0x73, 0xab, 0x17, 0xaf, 0x69 } },
    2
};

struct GPUInfo {
    wstring name;
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC desc;
};

class GpuMapper {
private:
    // Helper: Simple logger for remote debugging (Kept PRIVATE)
    static void Log(const wstring& msg) {
        // Saves to the folder where the .exe is running
        wofstream logFile("gpu_debug.log", ios::app);
        if (logFile.is_open()) {
            logFile << msg << endl;
        }
    }

public: // *** CONTAINSCASEINSENSITIVE MOVED TO PUBLIC TO FIX C2248 ***
    // Helper: Case-insensitive substring find
    static bool ContainsCaseInsensitive(const wstring& source, const wstring& substring) {
        if (substring.empty()) return true;
        auto it = search(
            source.begin(), source.end(),
            substring.begin(), substring.end(),
            [](wchar_t c1, wchar_t c2) { return towlower(c1) == towlower(c2); }
        );
        return it != source.end();
    }

    static vector<GPUInfo> getAvailableGPUs() {
        vector<GPUInfo> gpus;
        ComPtr<IDXGIFactory1> factory;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return gpus;

        for (UINT i = 0;; i++) {
            ComPtr<IDXGIAdapter> adapter;
            if (FAILED(factory->EnumAdapters(i, &adapter))) break;
            DXGI_ADAPTER_DESC desc;
            adapter->GetDesc(&desc);
            gpus.push_back({ desc.Description, adapter, desc });
        }
        return gpus;
    }

    static std::optional<LUID> resolveLuidFromBus(uint32_t targetBusIndex, const wstring& nameHint = L"") {
        Log(L"--- Starting Bus Resolution ---");
        Log(L"Target Bus: " + to_wstring(targetBusIndex));
        Log(L"Name Hint: " + nameHint);

        HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVCLASS_DISPLAY, nullptr, nullptr, DIGCF_PRESENT);
        if (devInfo == INVALID_HANDLE_VALUE) {
            Log(L"Error: Failed to get device list.");
            return std::nullopt;
        }

        SP_DEVINFO_DATA devData = {};
        devData.cbSize = sizeof(devData);
        std::optional<LUID> result = std::nullopt;

        for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devData); ++i) {
            
            // 1. Get Location
            WCHAR location[256];
            if (!SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_LOCATION_INFORMATION,
                nullptr, (PBYTE)location, sizeof(location), nullptr)) {
                continue;
            }

            // 2. Get Name (for logging/verification)
            WCHAR friendlyName[256];
            SetupDiGetDeviceRegistryPropertyW(devInfo, &devData, SPDRP_DEVICEDESC, 
                nullptr, (PBYTE)friendlyName, sizeof(friendlyName), nullptr);

            wstring locStr = location;
            
            // Log what we found
            wstring logMsg = L"Found: [" + wstring(friendlyName) + L"] at [" + locStr + L"]";
            Log(logMsg);

            // 3. Parse Bus
            size_t pos = locStr.find(L"PCI bus ");
            if (pos != wstring::npos) {
                uint32_t currentBus = wcstoul(locStr.substr(pos + 8).c_str(), nullptr, 10);
                
                if (currentBus == targetBusIndex) {
                    Log(L"-> BUS MATCHED!");

                    // 4. Name Safety Check (Case Insensitive)
                    if (!nameHint.empty()) {
                        if (!ContainsCaseInsensitive(friendlyName, nameHint) && 
                            !ContainsCaseInsensitive(nameHint, friendlyName)) {
                            Log(L"-> Name mismatch, ignoring (Safety check).");
                            continue; 
                        }
                    }

                    // 5. Get LUID
                    LUID deviceLuid = {};
                    DEVPROPTYPE propType;
                    ULONG propSize = 0;

                    if (SetupDiGetDevicePropertyW(devInfo, &devData, &DEVPKEY_Device_Luid_Custom, 
                        &propType, (PBYTE)&deviceLuid, sizeof(deviceLuid), &propSize, 0)) {
                        
                        Log(L"-> LUID Retrieved successfully.");
                        result = deviceLuid;
                        break; 
                    } else {
                        Log(L"-> Failed to retrieve LUID property.");
                    }
                }
            }
        }

        SetupDiDestroyDeviceInfoList(devInfo);
        
        if (!result.has_value()) Log(L"Result: FAILED to find matching LUID.");
        else Log(L"Result: SUCCESS.");
        
        return result;
    }
};

class AdapterOption {
public:
    bool hasTargetAdapter{};
    LUID adapterLuid{};
    wstring target_name{};

    wstring selectBestGPU() {
        auto gpus = GpuMapper::getAvailableGPUs();
        if (gpus.empty()) return L"";
        sort(gpus.begin(), gpus.end(), [](const GPUInfo& a, const GPUInfo& b) {
            return a.desc.DedicatedVideoMemory > b.desc.DedicatedVideoMemory;
        });
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
            wstring namePart = adapterSpec.substr(0, comma);
            wstring busPart = adapterSpec.substr(comma + 1);
            
            // Strip whitespace from bus part if necessary
            busPart.erase(remove_if(busPart.begin(), busPart.end(), iswspace), busPart.end());

            uint32_t bus = wcstoul(busPart.c_str(), nullptr, 10);
            
            auto luidOpt = GpuMapper::resolveLuidFromBus(bus, namePart);
            if (luidOpt.has_value()) {
                adapterLuid = luidOpt.value();
                hasTargetAdapter = true;
                return true;
            }
        } else {
            // Legacy Name-only match
            auto gpus = GpuMapper::getAvailableGPUs();
            for (const auto& gpu : gpus) {
                // Use the now-public GpuMapper::ContainsCaseInsensitive
                if (GpuMapper::ContainsCaseInsensitive(gpu.name, adapterSpec)) {
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