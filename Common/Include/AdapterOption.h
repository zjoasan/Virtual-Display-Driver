#pragma once

#include <wrl/client.h>
#include <dxgi.h>
#include <d3dkmt.h>
#include <setupapi.h>
#include <devpkey.h>
#include <cfgmgr32.h>
#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <comdef.h>

using namespace std;
using namespace Microsoft::WRL;

// Normalize PCI slot string from hex to decimal
inline std::wstring NormalizeSlotHexToDecimal(const std::wstring& hexSlot) {
    return std::to_wstring(std::wcstoul(hexSlot.c_str(), nullptr, 16));
}

// Extract slot from LocationInfo string (e.g., "PCI bus 1, device 0, function 0")
inline std::wstring ExtractSlotFromLocationInfo(const std::wstring& locationInfo) {
    size_t pos = locationInfo.find(L"function ");
    if (pos != std::wstring::npos) {
        std::wstring func = locationInfo.substr(pos + 9); // length of "function "
        return func;
    }
    return L"";
}

inline std::wstring NormalizeSlotInfo(const std::wstring& rawSlot, bool isHex) {
    return isHex ? NormalizeSlotHexToDecimal(rawSlot) : ExtractSlotFromLocationInfo(rawSlot);
}

std::wstring FormatBusDeviceFunction(UINT bus, UINT device, UINT function) {
    return std::to_wstring(bus) + L"." + std::to_wstring(device) + L"." + std::to_wstring(function); // e.g., "1.0.0"
}

// Structure to hold GPU information
struct GPUInfo {
    wstring name;
    ComPtr<IDXGIAdapter> adapter;
    DXGI_ADAPTER_DESC desc;
    wstring deviceId;
    wstring slotInfo;
};

// Sort GPUs by VRAM
bool CompareGPUs(const GPUInfo& a, const GPUInfo& b) {
    return a.desc.DedicatedVideoMemory > b.desc.DedicatedVideoMemory;
}

// Detect Windows version
bool IsWindows11OrGreater() {
    OSVERSIONINFOEXW osvi = { sizeof(osvi), 10, 0, 22000 }; // Win11 starts at build 22000
    DWORDLONG mask = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
    mask = VerSetConditionMask(mask, VER_MINORVERSION, VER_GREATER_EQUAL);
    mask = VerSetConditionMask(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
    return VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, mask);
}

// Adapter info from LUID
struct AdapterDeviceInfo {
    wstring deviceId;
    wstring slotInfo;
    LUID luid;
};

std::wstring GetSlotInfoFromDeviceId(const std::wstring& deviceId, HDEVINFO devInfo, SP_DEVINFO_DATA& devInfoData) {
    std::wstring slotInfo = L"Slot Unknown";
    DEVINST devInst;

    // Try DEVPKEY_Device_LocationInfo
    if (CM_Locate_DevNodeW(&devInst, const_cast<LPWSTR>(deviceId.c_str()), CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS) {
        DEVPROPTYPE propType;
        ULONG propSize = 0;
        if (CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_LocationInfo, &propType, nullptr, &propSize, 0) == CR_SUCCESS) {
            std::vector<BYTE> buffer(propSize);
            if (CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_LocationInfo, &propType, buffer.data(), &propSize, 0) == CR_SUCCESS) {
                return reinterpret_cast<PWCHAR>(buffer.data());
            }
        }
    }

    // Fallback: SPDRP_LOCATION_INFORMATION
    WCHAR legacyLocation[256];
    if (SetupDiGetDeviceRegistryPropertyW(devInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, nullptr,
        (PBYTE)legacyLocation, sizeof(legacyLocation), nullptr)) {
        return legacyLocation;
    }

    // Fallback: Parse from InstanceId
    size_t pos = deviceId.rfind(L"\\");
    if (pos != std::wstring::npos) {
        std::wstring tail = deviceId.substr(pos + 1); // e.g., "4&2C1B8F3&0&0008"
        if (tail.length() >= 4) {
            std::wstring pciHint = tail.substr(tail.length() - 4); // "0008"
            slotInfo = L"PCI Hint " + pciHint;
        }
    }

    return slotInfo;
}


vector<AdapterDeviceInfo> GetAdapterDeviceInfos() {
    vector<AdapterDeviceInfo> adapterInfos;

    const GUID& guid = GUID_DEVINTERFACE_VIDEO; // More reliable than DISPLAY_ADAPTER
    HDEVINFO devInfo = SetupDiGetClassDevsW(&guid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE) return adapterInfos;

    SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(SP_DEVICE_INTERFACE_DATA) };
    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(devInfo, nullptr, &guid, i, &interfaceData); ++i) {
        DWORD requiredSize = 0;
        SetupDiGetDeviceInterfaceDetailW(devInfo, &interfaceData, nullptr, 0, &requiredSize, nullptr);
        vector<BYTE> buffer(requiredSize);
        auto detail = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_W>(buffer.data());
        detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);
        if (!SetupDiGetDeviceInterfaceDetailW(devInfo, &interfaceData, detail, requiredSize, nullptr, nullptr)) continue;

        D3DKMT_OPENADAPTERFROMDEVICENAME openAdapter = {};
        openAdapter.pDeviceName = detail->DevicePath;
        if (!NT_SUCCESS(D3DKMTOpenAdapterFromDeviceName(&openAdapter))) continue;

		// Get Device Instance ID
		ULONG propSize = 0;
		DEVPROPTYPE propType;
		CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_InstanceId, &propType, nullptr, &propSize, 0);
		vector<BYTE> propBuffer(propSize);
		wstring deviceId;
		if (CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_InstanceId, &propType, propBuffer.data(), &propSize, 0) == CR_SUCCESS) {
			deviceId = reinterpret_cast<PWCHAR>(propBuffer.data());
		}

		// Prepare SP_DEVINFO_DATA for registry fallback
		SP_DEVINFO_DATA devInfoData = { sizeof(SP_DEVINFO_DATA) };
		if (!SetupDiOpenDeviceInfoW(devInfo, deviceId.c_str(), nullptr, 0, &devInfoData)) {
			slotInfo = L"Slot Unknown";
		} else {
			// Try DEVPKEY_Device_LocationInfo via devnode
			DEVINST devInst;
			slotInfo = L"Slot Unknown";
			if (CM_Locate_DevNodeW(&devInst, const_cast<LPWSTR>(deviceId.c_str()), CM_LOCATE_DEVNODE_NORMAL) == CR_SUCCESS) {
				propSize = 0;
				if (CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_LocationInfo, &propType, nullptr, &propSize, 0) == CR_SUCCESS) {
					propBuffer.resize(propSize);
					if (CM_Get_DevNode_PropertyW(devInst, &DEVPKEY_Device_LocationInfo, &propType, propBuffer.data(), &propSize, 0) == CR_SUCCESS) {
						slotInfo = reinterpret_cast<PWCHAR>(propBuffer.data());
						slotInfo = NormalizeSlotInfo(slotInfo, false);
					}
				}
			}

			// Fallback: SPDRP_LOCATION_INFORMATION
			if (slotInfo == L"Slot Unknown") {
				WCHAR legacyLocation[256];
				if (SetupDiGetDeviceRegistryPropertyW(devInfo, &devInfoData, SPDRP_LOCATION_INFORMATION, nullptr,
					(PBYTE)legacyLocation, sizeof(legacyLocation), nullptr)) {
					slotInfo = legacyLocation;
					slotInfo = NormalizeSlotInfo(slotInfo, false);
				}
			}

			// Fallback: parse from InstanceId
			if (slotInfo == L"Slot Unknown") {
				size_t pos = deviceId.rfind(L"\\");
				if (pos != wstring::npos) {
					wstring tail = deviceId.substr(pos + 1);
					if (tail.length() >= 4) {
						wstring pciHint = tail.substr(tail.length() - 4);
						slotInfo = NormalizeSlotInfo(pciHint, true);
					}
				}
			}
			
			// Final fallback: extract bus.device.function
			if (slotInfo == L"Slot Unknown") {
				ULONG propSize = 0;
				DEVPROPTYPE propType;
				UINT bus = 0, device = 0, function = 0;

				if (SetupDiGetDeviceProperty(devInfo, &devInfoData, &DEVPKEY_Device_BusNumber, &propType, (PBYTE)&bus, sizeof(bus), &propSize, 0) &&
					SetupDiGetDeviceProperty(devInfo, &devInfoData, &DEVPKEY_Device_Address, &propType, (PBYTE)&device, sizeof(device), &propSize, 0) &&
					SetupDiGetDeviceProperty(devInfo, &devInfoData, &DEVPKEY_Device_FunctionNumber, &propType, (PBYTE)&function, sizeof(function), &propSize, 0)) {
					slotInfo = FormatBusDeviceFunction(bus, device, function);
				}
			}
		}

        adapterInfos.push_back({ deviceId, slotInfo, openAdapter.AdapterLuid });
        D3DKMT_CLOSEADAPTER closeAdapter = { openAdapter.hAdapter };
        D3DKMTCloseAdapter(&closeAdapter);
    }

    SetupDiDestroyDeviceInfoList(devInfo);
    return adapterInfos;
}

// Enumerate GPUs
vector<GPUInfo> getAvailableGPUs() {
    vector<GPUInfo> gpus;
    ComPtr<IDXGIFactory1> factory;
    if (!SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) return gpus;

    auto deviceInfos = GetAdapterDeviceInfos();
    for (UINT i = 0;; i++) {
        ComPtr<IDXGIAdapter> adapter;
        if (!SUCCEEDED(factory->EnumAdapters(i, &adapter))) break;

        DXGI_ADAPTER_DESC desc;
        if (!SUCCEEDED(adapter->GetDesc(&desc))) continue;

        wstring deviceId, slotInfo;
        for (const auto& info : deviceInfos) {
            if (RtlEqualLuid(&desc.AdapterLuid, &info.luid)) {
                deviceId = info.deviceId;
                slotInfo = info.slotInfo;
                break;
            }
        }

        gpus.push_back({ desc.Description, adapter, desc, deviceId, slotInfo });
    }

    return gpus;
}

// Adapter selection class
class AdapterOption {
public:
    bool hasTargetAdapter{};
    LUID adapterLuid{};
    wstring target_name{};
    wstring target_device_id{};

	void xmlprovide(const std::wstring& xtarg) {
		target_name = xtarg;

		// Parse into name and slot
		auto pos = target_name.find(L',');
		wstring parsedName = (pos == wstring::npos) ? target_name : target_name.substr(0, pos);
		wstring parsedSlot = (pos == wstring::npos) ? L"" : target_name.substr(pos + 1);

		wstring combined = parsedName + (parsedSlot.empty() ? L"" : L"," + parsedSlot);


		if (!findAndSetAdapter(combined)) {
			target_name = selectBestGPU();
			findAndSetAdapter(target_name);
		}
	}

    wstring selectBestGPU() {
        auto gpus = getAvailableGPUs();
        if (gpus.empty()) return L"";
        sort(gpus.begin(), gpus.end(), CompareGPUs);
        auto& best = gpus.front();
        return best.name + (best.slotInfo.empty() ? L"" : L"," + best.slotInfo);
    }

	bool findAndSetAdapter(const wstring& target) {
		auto gpus = getAvailableGPUs();
		if (gpus.empty()) {
			hasTargetAdapter = false;
			return false;
		}

		// Parse name and optional slot
		auto pos = target.find(L',');
		wstring targetName = (pos == wstring::npos) ? target : target.substr(0, pos);
		wstring targetSlot = (pos == wstring::npos) ? L"" : target.substr(pos + 1);
		bool isTriplet = targetSlot.find(L'.') != std::wstring::npos;

		// Find all GPUs matching the friendly name
		vector<GPUInfo> matches;
		for (const auto& gpu : gpus) {
			if (_wcsicmp(gpu.name.c_str(), targetName.c_str()) == 0) {
				matches.push_back(gpu);
			}
		}

		if (matches.empty()) {
			hasTargetAdapter = false;
			return false;
		}

		// If only one match, ignore slot
		if (matches.size() == 1) {
			const auto& gpu = matches.front();
			adapterLuid = gpu.desc.AdapterLuid;
			target_device_id = gpu.deviceId;
			hasTargetAdapter = true;
			return true;
		}

		// Multiple matches: use slot if provided
		if (!targetSlot.empty()) {
			std::wstring normalizedTarget = NormalizeSlotInfo(targetSlot, isTriplet ? false : true);
			for (const auto& gpu : matches) {
				std::wstring normalizedGPU = NormalizeSlotInfo(gpu.slotInfo, isTriplet ? false : true);
				if (_wcsicmp(normalizedGPU.c_str(), normalizedTarget.c_str()) == 0) {
					adapterLuid = gpu.desc.AdapterLuid;
					target_device_id = gpu.deviceId;
					hasTargetAdapter = true;
					return true;
				}
			}
		}

		// No slot match or slot not provided: fallback to first match
		const auto& fallback = matches.front();
		adapterLuid = fallback.desc.AdapterLuid;
		target_device_id = fallback.deviceId;
		hasTargetAdapter = true;
		return true;
	}
};
