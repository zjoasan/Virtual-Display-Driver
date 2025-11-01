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
#include <msxml6.h> // For XML parsing

using namespace std;
using namespace Microsoft::WRL;

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

        // Get PCI slot info or fallback
        wstring slotInfo;
        if (CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_LocationInfo, &propType, nullptr, &propSize, 0) == CR_SUCCESS) {
            propBuffer.resize(propSize);
            if (CM_Get_Device_Interface_PropertyW(detail->DevicePath, &DEVPKEY_Device_LocationInfo, &propType, propBuffer.data(), &propSize, 0) == CR_SUCCESS) {
                slotInfo = reinterpret_cast<PWCHAR>(propBuffer.data());
            }
        } else {
            slotInfo = L"Slot Unknown";
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

	void loadFromXML(const wchar_t* xmlPath) {
		ComPtr<IXMLDOMDocument> doc;
		doc.CoCreateInstance(__uuidof(DOMDocument60));
		VARIANT_BOOL loaded;
		if (SUCCEEDED(doc->load(_variant_t(xmlPath), &loaded)) && loaded == VARIANT_TRUE) {
			ComPtr<IXMLDOMNode> node;
			doc->selectSingleNode(_bstr_t(L"//friendlyname"), &node);
			if (node) {
				BSTR value;
				node->get_text(&value);
				target_name = value;
				SysFreeString(value);
			}
		}

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
			for (const auto& gpu : matches) {
				if (_wcsicmp(gpu.slotInfo.c_str(), targetSlot.c_str()) == 0) {
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
