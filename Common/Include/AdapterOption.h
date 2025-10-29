#pragma once

#include <wrl/client.h>     // For ComPtr
#include <dxgi.h>           // For IDXGIAdapter, IDXGIFactory1
#include <algorithm>        // For sort
#include <d3dkmt.h>         // For D3DKMT functions
#include <setupapi.h>       // For SetupAPI
#include <devpkey.h>        // For DEVPKEY
#include <cfgmgr32.h>       // For CM_ functions
#include <fstream>          // For file I/O
#include <string>

using namespace std;
using namespace Microsoft::WRL;

// Structure to hold GPU information
struct GPUInfo {
    wstring name;                // GPU friendly name (from DXGI_ADAPTER_DESC)
    ComPtr<IDXGIAdapter> adapter;// COM pointer to the adapter
    DXGI_ADAPTER_DESC desc;      // Adapter description
    wstring deviceId;            // Device Instance ID
    wstring slotInfo;            // PCI slot information (e.g., "PCI Slot 2")
};

// Sort function for GPUs by dedicated video memory
bool CompareGPUs(const GPUInfo& a, const GPUInfo& b) {
    return a.desc.DedicatedVideoMemory > b.desc.DedicatedVideoMemory;
}

// Helper function to get Device Instance ID and PCI slot info from LUID
struct AdapterDeviceInfo {
    wstring deviceId;
    wstring slotInfo;
    LUID luid;
};

vector<AdapterDeviceInfo> GetAdapterDeviceInfos() {
    vector<AdapterDeviceInfo> adapterInfos;

    // Enumerate display adapter interfaces
    HDEVINFO devInfo = SetupDiGetClassDevsW(&GUID_DEVINTERFACE_DISPLAY_ADAPTER, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE) {
        return adapterInfos;
    }

    SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &GUID_DEVINTERFACE_DISPLAY_ADAPTER, i, &interfaceData); ++i) {
        // Get device interface path
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);
        vector<WCHAR> buffer(requiredSize / sizeof(WCHAR));
        PSP_DEVICE_INTERFACE_DETAIL_DATA_W detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &interfaceData, detail, requiredSize, nullptr, nullptr)) {
            continue;
        }

        // Get LUID
        D3DKMT_OPENADAPTERFROMDEVICENAME openAdapter = {};
        openAdapter.pDeviceName = detail->DevicePath;
        NTSTATUS status = D3DKMTOpenAdapterFromDeviceName(&openAdapter);
        if (!NT_SUCCESS(status)) {
            continue;
        }

        // Get Device Instance ID
        ULONG propSize = 0;
        DEVPROPTYPE propType;
        CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_InstanceId, &propType, nullptr, &propSize, 0);
        vector<BYTE> propBuffer(propSize);
        wstring deviceId;
        if (CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_InstanceId, &propType, propBuffer.data(), &propSize, 0) == CR_SUCCESS) {
            deviceId = reinterpret_cast<PWCHAR>(propBuffer.data());
        }

        // Get PCI slot info
        propSize = 0;
        CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_LocationInfo, &propType, nullptr, &propSize, 0);
        propBuffer.resize(propSize);
        wstring slotInfo;
        if (CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_LocationInfo, &propType, propBuffer.data(), &propSize, 0) == CR_SUCCESS) {
            slotInfo = reinterpret_cast<PWCHAR>(propBuffer.data());
        }

        adapterInfos.push_back({ deviceId, slotInfo, openAdapter.AdapterLuid });

        D3DKMT_CLOSEADAPTER closeAdapter = { openAdapter.hAdapter };
        D3DKMTCloseAdapter(&closeAdapter);
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return adapterInfos;
}

// Get a list of available GPUs with Device Instance ID and PCI slot info
vector<GPUInfo> getAvailableGPUs() {
    vector<GPUInfo> gpus;

    ComPtr<IDXGIFactory1> factory;
    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return gpus;
    }

    // Get Device Instance IDs and slot info
    auto deviceInfos = GetAdapterDeviceInfos();

    // Enumerate all adapters (GPUs)
    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter> adapter;
        if (!SUCCEEDED(factory->EnumAdapters(i, &adapter))) {
            break;
        }

        DXGI_ADAPTER_DESC desc;
        if (!SUCCEEDED(adapter->GetDesc(&desc))) {
            continue;
        }

        // Find matching Device Instance ID and slot info
        wstring deviceId, slotInfo;
        for (const auto& deviceInfo : deviceInfos) {
            if (RtlEqualLuid(&desc.AdapterLuid, &deviceInfo.luid)) {
                deviceId = deviceInfo.deviceId;
                slotInfo = deviceInfo.slotInfo;
                break;
            }
        }

        // Add the adapter information to the list
        GPUInfo info{ desc.Description, adapter, desc, deviceId, slotInfo };
        gpus.push_back(info);
    }

    return gpus;
}

class AdapterOption {
public:
    bool hasTargetAdapter{}; // Indicates if a target adapter is selected
    LUID adapterLuid{};      // Adapter's unique identifier (LUID)
    wstring target_name{};   // Target adapter name (friendlyname,slot)
    wstring target_device_id{}; // Device Instance ID of selected adapter

    // Select the best GPU based on dedicated video memory
    wstring selectBestGPU() {
        auto gpus = getAvailableGPUs();
        if (gpus.empty()) {
            return L""; // Error check for headless/VM
        }

        // Sort GPUs by dedicated video memory in descending order
        sort(gpus.begin(), gpus.end(), CompareGPUs);
        auto bestGPU = gpus.front(); // Get the GPU with the most memory

        // Return friendlyname,slot format
        return bestGPU.name + (bestGPU.slotInfo.empty() ? L"" : L"," + bestGPU.slotInfo);
    }

    // Parse friendlyname,pcislot input
    pair<wstring, wstring> parseTarget(const wstring& input) {
        auto pos = input.find(L',');
        if (pos == wstring::npos) {
            return { input, L"" }; // No slot specified
        }
        return { input.substr(0, pos), input.substr(pos + 1) }; // Friendly name, slot
    }

    // Load friendlyname,slot from a file or select the best GPU
    void load(const wchar_t* path) {
        ifstream ifs{ path };
        if (!ifs.is_open()) {
            target_name = selectBestGPU();
        }
        else {
            string line;
            getline(ifs, line);
            target_name.assign(line.begin(), line.end());
        }

        // Find and set the adapter based on the target name and slot
        if (!findAndSetAdapter(target_name)) {
            // If the adapter is not found, select the best GPU and retry
            target_name = selectBestGPU();
            findAndSetAdapter(target_name);
        }
    }

    // Set the target adapter from a given name and slot (e.g., "NVIDIA GeForce RTX 3080,PCI Slot 2")
    void xmlprovide(const wstring& xtarg) {
        target_name = xtarg;
        if (!findAndSetAdapter(target_name)) {
            // If the adapter is not found, select the best GPU and retry
            target_name = selectBestGPU();
            findAndSetAdapter(target_name);
        }
    }

    // Apply the adapter settings to the specified adapter
    void apply(const IDDCX_ADAPTER& adapter) {
        if (hasTargetAdapter && IDD_IS_FUNCTION_AVAILABLE(IddCxAdapterSetRenderAdapter)) {
            IDARG_IN_ADAPTERSETRENDERADAPTER arg{};
            arg.PreferredRenderAdapter = adapterLuid;
            IddCxAdapterSetRenderAdapter(adapter, &arg);
        }
    }

private:
    // Find and set the adapter by its friendly name and optional PCI slot
    bool findAndSetAdapter(const wstring& target) {
        auto gpus = getAvailableGPUs();
        if (gpus.empty()) {
            hasTargetAdapter = false;
            return false;
        }

        auto [targetName, targetSlot] = parseTarget(target);

        // Find all adapters matching the friendly name
        vector<GPUInfo> matchingGPUs;
        for (const auto& gpu : gpus) {
            if (_wcsicmp(gpu.name.c_str(), targetName.c_str()) == 0) {
                matchingGPUs.push_back(gpu);
            }
        }

        if (matchingGPUs.empty()) {
            hasTargetAdapter = false;
            return false;
        }

        // If only one match or no slot specified, use the first match
        if (matchingGPUs.size() == 1 || targetSlot.empty()) {
            const auto& gpu = matchingGPUs.front();
            adapterLuid = gpu.desc.AdapterLuid;
            target_device_id = gpu.deviceId;
            hasTargetAdapter = true;
            return true;
        }

        // Multiple matches: use PCI slot to disambiguate
        for (const auto& gpu : matchingGPUs) {
            if (_wcsicmp(gpu.slotInfo.c_str(), targetSlot.c_str()) == 0) {
                adapterLuid = gpu.desc.AdapterLuid;
                target_device_id = gpu.deviceId;
                hasTargetAdapter = true;
                return true;
            }
        }

        // No slot match found; fall back to first matching GPU
        const auto& gpu = matchingGPUs.front();
        adapterLuid = gpu.desc.AdapterLuid;
        target_device_id = gpu.deviceId;
        hasTargetAdapter = true;
        return true;
    }
};
