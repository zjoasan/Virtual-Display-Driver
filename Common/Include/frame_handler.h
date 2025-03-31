#ifndef FRAME_HANDLER_H
#define FRAME_HANDLER_H

#include <d3d11.h>
#include <dxgi1_2.h>
#include <DirectXMath.h>
#include <wdf.h>
#include <iddcx.h>

struct PerFrameBuffer {
    DirectX::XMFLOAT4 resolution;
    float time;
    float padding[3];
};

class FrameHandler {
public:
    static ID3D11Device* g_device;
    static ID3D11DeviceContext* g_deviceContext;
    static ID3D11VertexShader* g_vertexShader;
    static ID3D11PixelShader* g_pixelShader;
    static ID3D11InputLayout* g_inputLayout;
    static ID3D11Buffer* g_constantBuffer;
    static ID3D11RenderTargetView* g_outputRTV;
    static bool enabled;

    static void InitializeFrameHandler(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* shaderFilePath);
    static NTSTATUS AssignSwapChain(IDDCX_SWAPCHAIN hSwapChain, IDXGISwapChain* dxgiSwapChain);
    static void ProcessFrame(IDDCX_SWAPCHAIN hSwapChain, LARGE_INTEGER PresentationTime);
    static bool IsEnabled() { return enabled; }
};

#endif