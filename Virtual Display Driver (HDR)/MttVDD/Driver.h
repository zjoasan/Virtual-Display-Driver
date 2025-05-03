#pragma once

#define NOMINMAX
#include <windows.h>
#include <bugcodes.h>
#include <wudfwdm.h>
#include <wdf.h>
#include <iddcx.h>

#include <dxgi1_5.h>
#include <d3d11_2.h>
#include <avrt.h>
#include <wrl.h>

#include <memory>
#include <vector>

#include "Trace.h"
#include "mttvdd-ioctl.h"
#include "Common.h"

// Forward declaration for the IndirectDeviceContext class
namespace Microsoft
{
    namespace IndirectDisp
    {
        class IndirectDeviceContext;
    }
}

// Forward declarations for driver-defined context type
struct IndirectDeviceContextWrapper
{
    Microsoft::IndirectDisp::IndirectDeviceContext* pContext;

    void Cleanup();  // Just declare it, don't implement
};

// The WDF context access function is defined by WDF_DECLARE_CONTEXT_TYPE macro
WDF_DECLARE_CONTEXT_TYPE(IndirectDeviceContextWrapper);

namespace Microsoft
{
    namespace WRL
    {
        namespace Wrappers
        {
            // Adds a wrapper for thread handles to the existing set of WRL handle wrapper classes
            typedef HandleT<HandleTraits::HANDLENullTraits> Thread;
        }
    }
}

namespace Microsoft
{
    namespace IndirectDisp
    {
        /// <summary>
        /// Manages the creation and lifetime of a Direct3D render device.
        /// </summary>
        struct Direct3DDevice
        {
            Direct3DDevice(LUID AdapterLuid);
            Direct3DDevice();
            HRESULT Init();

            LUID AdapterLuid;
            Microsoft::WRL::ComPtr<IDXGIFactory5> DxgiFactory;
            Microsoft::WRL::ComPtr<IDXGIAdapter1> Adapter;
            Microsoft::WRL::ComPtr<ID3D11Device> Device;
            Microsoft::WRL::ComPtr<ID3D11DeviceContext> DeviceContext;
        };

        /// <summary>
        /// Manages a thread that consumes buffers from an indirect display swap-chain object.
        /// </summary>
        class SwapChainProcessor
        {
        public:
            SwapChainProcessor(IDDCX_SWAPCHAIN hSwapChain, std::shared_ptr<Direct3DDevice> Device, HANDLE NewFrameEvent);
            ~SwapChainProcessor();

        private:
            static DWORD CALLBACK RunThread(LPVOID Argument);

            void Run();
            void RunCore();

        public:
            IDDCX_SWAPCHAIN m_hSwapChain;
            std::shared_ptr<Direct3DDevice> m_Device;
            HANDLE m_hAvailableBufferEvent;
            Microsoft::WRL::Wrappers::Thread m_hThread;
            Microsoft::WRL::Wrappers::Event m_hTerminateEvent;
        };

        /// <summary>
        /// Provides a sample implementation of an indirect display driver.
        /// </summary>
        class IndirectDeviceContext
        {
        public:
            IndirectDeviceContext(_In_ WDFDEVICE WdfDevice);
            virtual ~IndirectDeviceContext();

            void InitAdapter();
            void FinishInit();

            void CreateMonitor(unsigned int index);
            NTSTATUS CreateMonitorWithParams(ULONG Width, ULONG Height, ULONG RefreshRate, GUID* MonitorId);
            NTSTATUS DestroyMonitor(ULONG MonitorIndex);
            NTSTATUS SetMonitorMode(ULONG MonitorIndex, ULONG Width, ULONG Height, ULONG RefreshRate);
            NTSTATUS SetPreferredMode(ULONG MonitorIndex, ULONG Width, ULONG Height, ULONG RefreshRate);
            NTSTATUS SetGpuPreference(LUID AdapterLuid);
            ULONG GetMonitorCount();

            void AssignSwapChain(IDDCX_MONITOR& Monitor, IDDCX_SWAPCHAIN SwapChain, LUID RenderAdapter, HANDLE NewFrameEvent);
            void UnassignSwapChain();

        protected:
            WDFDEVICE m_WdfDevice;
            IDDCX_ADAPTER m_Adapter;
            IDDCX_MONITOR m_Monitor;
            IDDCX_MONITOR m_Monitor2;

            std::unique_ptr<SwapChainProcessor> m_ProcessingThread;
            
            // Store a list of connected monitors and their info
            std::vector<std::pair<IDDCX_MONITOR, GUID>> m_Monitors;

        public:
            static const DISPLAYCONFIG_VIDEO_SIGNAL_INFO s_KnownMonitorModes[];
            static std::vector<BYTE> s_KnownMonitorEdid;
        };
    }
}