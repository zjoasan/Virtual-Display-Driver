#include "Driver.h"
#include "Common.h"
#include <sstream>

using namespace Microsoft::IndirectDisp;
using namespace std;

// External declaration of the logging function
extern void vddlog(const char* type, const char* message);

// Forward declaration of callback function
extern "C" EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MttVddEvtIoDeviceControl;

NTSTATUS IndirectDeviceContext::CreateMonitorWithParams(ULONG Width, ULONG Height, ULONG RefreshRate, GUID* MonitorId)
{
    // Log the parameters
    stringstream logStream;
    logStream << "Creating monitor with parameters:"
        << "\n  Width: " << Width
        << "\n  Height: " << Height 
        << "\n  RefreshRate: " << RefreshRate
        << "\n  MonitorId: " << (MonitorId ? "Provided" : "Not provided");
    vddlog("i", logStream.str().c_str());
    
    // If MonitorId is null, generate a new one
    GUID monitorGuid;
    if (MonitorId == nullptr || IsEqualGUID(*MonitorId, GUID_NULL))
    {
        CoCreateGuid(&monitorGuid);
        logStream.str("");
        logStream << "Generated new monitor GUID";
        vddlog("i", logStream.str().c_str());
    }
    else
    {
        monitorGuid = *MonitorId;
        logStream.str("");
        logStream << "Using provided monitor GUID";
        vddlog("i", logStream.str().c_str());
    }

    // TODO: Implement actual monitor creation with the specified parameters
    // For now, we'll add a placeholder that just uses the existing CreateMonitor function
    // This would be enhanced to use the width, height, and refresh rate parameters

    // Add the monitor to our list
    unsigned int index = (unsigned int)m_Monitors.size();
    CreateMonitor(index);

    // Store the GUID with the monitor for future reference
    m_Monitors.push_back(std::make_pair(m_Monitor, monitorGuid));

    logStream.str("");
    logStream << "Monitor created successfully with index: " << index;
    vddlog("i", logStream.str().c_str());

    return STATUS_SUCCESS;
}

NTSTATUS IndirectDeviceContext::DestroyMonitor(ULONG MonitorIndex)
{
    // Log the request
    stringstream logStream;
    logStream << "Destroying monitor with index: " << MonitorIndex;
    vddlog("i", logStream.str().c_str());

    // Check if the index is valid
    if (MonitorIndex >= m_Monitors.size())
    {
        logStream.str("");
        logStream << "Invalid monitor index: " << MonitorIndex << ". Max index: " << (m_Monitors.size() > 0 ? m_Monitors.size() - 1 : 0);
        vddlog("e", logStream.str().c_str());
        return STATUS_INVALID_PARAMETER;
    }

    // TODO: Implement actual monitor destruction logic
    // For now, this is a placeholder that would be enhanced in a real implementation
    
    // Remove the monitor from our list
    m_Monitors.erase(m_Monitors.begin() + MonitorIndex);

    logStream.str("");
    logStream << "Monitor with index " << MonitorIndex << " successfully destroyed";
    vddlog("i", logStream.str().c_str());

    return STATUS_SUCCESS;
}

void IndirectDeviceContextWrapper::Cleanup()
{
    delete pContext;
    pContext = nullptr;
}

NTSTATUS IndirectDeviceContext::SetMonitorMode(ULONG MonitorIndex, ULONG Width, ULONG Height, ULONG RefreshRate)
{
    // Log the request
    stringstream logStream;
    logStream << "Setting monitor mode:"
        << "\n  MonitorIndex: " << MonitorIndex
        << "\n  Width: " << Width
        << "\n  Height: " << Height
        << "\n  RefreshRate: " << RefreshRate;
    vddlog("i", logStream.str().c_str());
    
    // Check if the index is valid
    if (MonitorIndex >= m_Monitors.size())
    {
        logStream.str("");
        logStream << "Invalid monitor index: " << MonitorIndex << ". Max index: " << (m_Monitors.size() > 0 ? m_Monitors.size() - 1 : 0);
        vddlog("e", logStream.str().c_str());
        return STATUS_INVALID_PARAMETER;
    }

    // TODO: Implement logic to change the monitor mode
    // This would need to interact with the IddCx APIs to change the resolution/refresh rate

    logStream.str("");
    logStream << "Monitor mode set successfully for monitor index " << MonitorIndex;
    vddlog("i", logStream.str().c_str());

    return STATUS_SUCCESS;
}

NTSTATUS IndirectDeviceContext::SetPreferredMode(ULONG MonitorIndex, ULONG Width, ULONG Height, ULONG RefreshRate)
{
    // Log the request
    stringstream logStream;
    logStream << "Setting preferred monitor mode:"
        << "\n  MonitorIndex: " << MonitorIndex
        << "\n  Width: " << Width
        << "\n  Height: " << Height
        << "\n  RefreshRate: " << RefreshRate;
    vddlog("i", logStream.str().c_str());
    
    // Check if the index is valid
    if (MonitorIndex >= m_Monitors.size())
    {
        logStream.str("");
        logStream << "Invalid monitor index: " << MonitorIndex << ". Max index: " << (m_Monitors.size() > 0 ? m_Monitors.size() - 1 : 0);
        vddlog("e", logStream.str().c_str());
        return STATUS_INVALID_PARAMETER;
    }

    // TODO: Implement logic to set the preferred mode for the monitor
    // This would update the EDID or other configuration to indicate the preferred mode

    logStream.str("");
    logStream << "Preferred mode set successfully for monitor index " << MonitorIndex;
    vddlog("i", logStream.str().c_str());

    return STATUS_SUCCESS;
}

NTSTATUS IndirectDeviceContext::SetGpuPreference(LUID AdapterLuid)
{
    // Log the request
    stringstream logStream;
    logStream << "Setting GPU preference:"
        << "\n  AdapterLuid.LowPart: " << AdapterLuid.LowPart
        << "\n  AdapterLuid.HighPart: " << AdapterLuid.HighPart;
    vddlog("i", logStream.str().c_str());

    // TODO: Implement GPU preference setting
    // This would call IddCxAdapterSetRenderAdapter or similar

    logStream.str("");
    logStream << "GPU preference set successfully";
    vddlog("i", logStream.str().c_str());

    return STATUS_SUCCESS;
}

ULONG IndirectDeviceContext::GetMonitorCount()
{
    ULONG count = (ULONG)m_Monitors.size();
    
    // Log the request
    stringstream logStream;
    logStream << "Getting monitor count: " << count << " monitor(s)";
    vddlog("i", logStream.str().c_str());
    
    return count;
}

// IOCTL handler implementation
VOID MttVddEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    // Log the IOCTL request
    stringstream logStream;
    logStream << "Received IOCTL request:"
        << "\n  IoControlCode: 0x" << std::hex << IoControlCode << std::dec
        << "\n  InputBufferLength: " << InputBufferLength 
        << "\n  OutputBufferLength: " << OutputBufferLength;
    vddlog("i", logStream.str().c_str());

    NTSTATUS status = STATUS_SUCCESS;
    WDFDEVICE device = WdfIoQueueGetDevice(Queue);
    
    // Get the device context
    IndirectDeviceContextWrapper* pContextWrapper = WdfObjectGet_IndirectDeviceContextWrapper(device);
    if (!pContextWrapper || !pContextWrapper->pContext)
    {
        logStream.str("");
        logStream << "Invalid device context - request will be completed with STATUS_INVALID_DEVICE_STATE";
        vddlog("e", logStream.str().c_str());
        
        WdfRequestComplete(Request, STATUS_INVALID_DEVICE_STATE);
        return;
    }
    
    Microsoft::IndirectDisp::IndirectDeviceContext* pContext = pContextWrapper->pContext;

    PVOID inputBuffer = nullptr;
    PVOID outputBuffer = nullptr;
    size_t bytesReturned = 0;

    // Get input and output buffers if needed
    if (InputBufferLength > 0)
    {
        status = WdfRequestRetrieveInputBuffer(Request, InputBufferLength, &inputBuffer, nullptr);
        if (!NT_SUCCESS(status))
        {
            logStream.str("");
            logStream << "Failed to retrieve input buffer with status: 0x" << std::hex << status;
            vddlog("e", logStream.str().c_str());
            
            WdfRequestComplete(Request, status);
            return;
        }
        
        logStream.str("");
        logStream << "Successfully retrieved input buffer";
        vddlog("i", logStream.str().c_str());
    }

    if (OutputBufferLength > 0)
    {
        status = WdfRequestRetrieveOutputBuffer(Request, OutputBufferLength, &outputBuffer, nullptr);
        if (!NT_SUCCESS(status))
        {
            logStream.str("");
            logStream << "Failed to retrieve output buffer with status: 0x" << std::hex << status;
            vddlog("e", logStream.str().c_str());
            
            WdfRequestComplete(Request, status);
            return;
        }
        
        logStream.str("");
        logStream << "Successfully retrieved output buffer";
        vddlog("i", logStream.str().c_str());
    }

    // Process the IOCTL based on the control code
    logStream.str("");
    logStream << "Processing IOCTL control code: 0x" << std::hex << IoControlCode << std::dec;
    vddlog("i", logStream.str().c_str());
    
    switch (IoControlCode)
    {
    case IOCTL_MTTVDD_GET_VERSION:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_GET_VERSION request received";
            vddlog("i", logStream.str().c_str());
            
            if (OutputBufferLength < sizeof(MTTVDD_VERSION_INFO))
            {
                logStream.str("");
                logStream << "Output buffer too small: " << OutputBufferLength << " bytes (required " << sizeof(MTTVDD_VERSION_INFO) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMTTVDD_VERSION_INFO versionInfo = (PMTTVDD_VERSION_INFO)outputBuffer;
            versionInfo->MajorVersion = 1;
            versionInfo->MinorVersion = 0;
            versionInfo->BuildNumber = 1;
            bytesReturned = sizeof(MTTVDD_VERSION_INFO);
            
            logStream.str("");
            logStream << "Version information returned: " 
                << versionInfo->MajorVersion << "."
                << versionInfo->MinorVersion << "."
                << versionInfo->BuildNumber;
            vddlog("i", logStream.str().c_str());
        }
        break;

    case IOCTL_MTTVDD_CREATE_MONITOR:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_CREATE_MONITOR request received";
            vddlog("i", logStream.str().c_str());
            
            if (InputBufferLength < sizeof(MTTVDD_CREATE_MONITOR))
            {
                logStream.str("");
                logStream << "Input buffer too small: " << InputBufferLength << " bytes (required " << sizeof(MTTVDD_CREATE_MONITOR) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMTTVDD_CREATE_MONITOR createInfo = (PMTTVDD_CREATE_MONITOR)inputBuffer;
            
            logStream.str("");
            logStream << "Calling CreateMonitorWithParams with:"
                << "\n  Width: " << createInfo->Width
                << "\n  Height: " << createInfo->Height
                << "\n  RefreshRate: " << createInfo->RefreshRate;
            vddlog("i", logStream.str().c_str());
            
            status = pContext->CreateMonitorWithParams(createInfo->Width, createInfo->Height, createInfo->RefreshRate, &createInfo->MonitorId);
            
            logStream.str("");
            logStream << "CreateMonitorWithParams returned status: 0x" << std::hex << status;
            vddlog("i", logStream.str().c_str());
        }
        break;

    case IOCTL_MTTVDD_DESTROY_MONITOR:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_DESTROY_MONITOR request received";
            vddlog("i", logStream.str().c_str());
            
            if (InputBufferLength < sizeof(MTTVDD_DESTROY_MONITOR))
            {
                logStream.str("");
                logStream << "Input buffer too small: " << InputBufferLength << " bytes (required " << sizeof(MTTVDD_DESTROY_MONITOR) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMTTVDD_DESTROY_MONITOR destroyInfo = (PMTTVDD_DESTROY_MONITOR)inputBuffer;
            
            logStream.str("");
            logStream << "Calling DestroyMonitor with index: " << destroyInfo->MonitorIndex;
            vddlog("i", logStream.str().c_str());
            
            status = pContext->DestroyMonitor(destroyInfo->MonitorIndex);
            
            logStream.str("");
            logStream << "DestroyMonitor returned status: 0x" << std::hex << status;
            vddlog("i", logStream.str().c_str());
        }
        break;

    case IOCTL_MTTVDD_SET_MODE:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_SET_MODE request received";
            vddlog("i", logStream.str().c_str());
            
            if (InputBufferLength < sizeof(MTTVDD_SET_MODE))
            {
                logStream.str("");
                logStream << "Input buffer too small: " << InputBufferLength << " bytes (required " << sizeof(MTTVDD_SET_MODE) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMTTVDD_SET_MODE modeInfo = (PMTTVDD_SET_MODE)inputBuffer;
            
            logStream.str("");
            logStream << "Calling SetMonitorMode with:"
                << "\n  MonitorIndex: " << modeInfo->MonitorIndex
                << "\n  Width: " << modeInfo->Width
                << "\n  Height: " << modeInfo->Height
                << "\n  RefreshRate: " << modeInfo->RefreshRate;
            vddlog("i", logStream.str().c_str());
            
            status = pContext->SetMonitorMode(modeInfo->MonitorIndex, modeInfo->Width, modeInfo->Height, modeInfo->RefreshRate);
            
            logStream.str("");
            logStream << "SetMonitorMode returned status: 0x" << std::hex << status;
            vddlog("i", logStream.str().c_str());
        }
        break;

    case IOCTL_MTTVDD_GET_MONITOR_COUNT:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_GET_MONITOR_COUNT request received";
            vddlog("i", logStream.str().c_str());
            
            if (OutputBufferLength < sizeof(ULONG))
            {
                logStream.str("");
                logStream << "Output buffer too small: " << OutputBufferLength << " bytes (required " << sizeof(ULONG) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PULONG count = (PULONG)outputBuffer;
            *count = pContext->GetMonitorCount();
            bytesReturned = sizeof(ULONG);
            
            logStream.str("");
            logStream << "Monitor count returned: " << *count;
            vddlog("i", logStream.str().c_str());
        }
        break;

    case IOCTL_MTTVDD_SET_PREFERRED_MODE:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_SET_PREFERRED_MODE request received";
            vddlog("i", logStream.str().c_str());
            
            if (InputBufferLength < sizeof(MTTVDD_SET_PREFERRED_MODE))
            {
                logStream.str("");
                logStream << "Input buffer too small: " << InputBufferLength << " bytes (required " << sizeof(MTTVDD_SET_PREFERRED_MODE) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMTTVDD_SET_PREFERRED_MODE modeInfo = (PMTTVDD_SET_PREFERRED_MODE)inputBuffer;
            
            logStream.str("");
            logStream << "Calling SetPreferredMode with:"
                << "\n  MonitorIndex: " << modeInfo->MonitorIndex
                << "\n  Width: " << modeInfo->Width
                << "\n  Height: " << modeInfo->Height
                << "\n  RefreshRate: " << modeInfo->RefreshRate;
            vddlog("i", logStream.str().c_str());
            
            status = pContext->SetPreferredMode(modeInfo->MonitorIndex, modeInfo->Width, modeInfo->Height, modeInfo->RefreshRate);
            
            logStream.str("");
            logStream << "SetPreferredMode returned status: 0x" << std::hex << status;
            vddlog("i", logStream.str().c_str());
        }
        break;

    case IOCTL_MTTVDD_SET_GPU_PREFERENCE:
        {
            logStream.str("");
            logStream << "IOCTL_MTTVDD_SET_GPU_PREFERENCE request received";
            vddlog("i", logStream.str().c_str());
            
            if (InputBufferLength < sizeof(MTTVDD_SET_GPU_PREFERENCE))
            {
                logStream.str("");
                logStream << "Input buffer too small: " << InputBufferLength << " bytes (required " << sizeof(MTTVDD_SET_GPU_PREFERENCE) << " bytes)";
                vddlog("e", logStream.str().c_str());
                
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            PMTTVDD_SET_GPU_PREFERENCE gpuInfo = (PMTTVDD_SET_GPU_PREFERENCE)inputBuffer;
            
            logStream.str("");
            logStream << "Calling SetGpuPreference with:"
                << "\n  AdapterLuid.LowPart: " << gpuInfo->AdapterLuid.LowPart
                << "\n  AdapterLuid.HighPart: " << gpuInfo->AdapterLuid.HighPart;
            vddlog("i", logStream.str().c_str());
            
            status = pContext->SetGpuPreference(gpuInfo->AdapterLuid);
            
            logStream.str("");
            logStream << "SetGpuPreference returned status: 0x" << std::hex << status;
            vddlog("i", logStream.str().c_str());
        }
        break;

    default:
        logStream.str("");
        logStream << "Unknown IOCTL code: 0x" << std::hex << IoControlCode;
        vddlog("e", logStream.str().c_str());
        
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    // Complete the request with the status and bytes returned
    logStream.str("");
    logStream << "Completing IOCTL request with:"
        << "\n  Status: 0x" << std::hex << status
        << "\n  BytesReturned: " << std::dec << bytesReturned;
    vddlog("i", logStream.str().c_str());
    
    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}

// Function to setup the IOCTL queue
NTSTATUS SetupIoctlQueue(WDFDEVICE Device)
{
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDFQUEUE queue;

    // Initialize the queue configuration
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);

    // Set the callback for device control IOCTLs
    queueConfig.EvtIoDeviceControl = MttVddEvtIoDeviceControl;

    // Create the queue
    NTSTATUS status = WdfIoQueueCreate(Device, &queueConfig, WDF_NO_OBJECT_ATTRIBUTES, &queue);
    
    return status;
}