#include "frame_handler.h"
#include <wdf.h>
#include <wudfwdm.h>
#include <iddcx.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <DirectXMath.h>
#include "globals.h"
#include "CRT_shader.h"

ID3D11Device* FrameHandler::g_device = nullptr;
ID3D11DeviceContext* FrameHandler::g_deviceContext = nullptr;
ID3D11VertexShader* FrameHandler::g_vertexShader = nullptr;
ID3D11PixelShader* FrameHandler::g_pixelShader = nullptr;
ID3D11InputLayout* FrameHandler::g_inputLayout = nullptr;
ID3D11Buffer* FrameHandler::g_constantBuffer = nullptr;
ID3D11RenderTargetView* FrameHandler::g_outputRTV = nullptr;
bool FrameHandler::enabled = false;

void FrameHandler::InitializeFrameHandler(ID3D11Device* device, ID3D11DeviceContext* context, const wchar_t* shaderfilepath) {
    UNREFERENCED_PARAMETER(shaderfilepath);
    g_device = device;
    g_deviceContext = context;
    enabled = false;

    std::wstring activeShaderName =  ShaderShortName;

    if (activeShaderName.empty() || activeShaderName == L"") {
        return;
    }

    if (activeShaderName == L"CRT") {
#include "CRT_shader.h"
        enabled = true;

        D3D11_INPUT_ELEMENT_DESC layout[] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };

        HRESULT hr = g_device->CreateInputLayout(layout, ARRAYSIZE(layout), CRTShader_VS, CRTShader_VS_size, &g_inputLayout);
        if (FAILED(hr)) return;

        hr = g_device->CreateVertexShader(CRTShader_VS, CRTShader_VS_size, nullptr, &g_vertexShader);
        if (FAILED(hr)) return;

        hr = g_device->CreatePixelShader(CRTShader_PS, CRTShader_PS_size, nullptr, &g_pixelShader);
        if (FAILED(hr)) return;
    }
    else {
        std::wstring header = activeShaderName + L"_shader.h";
    }

    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = sizeof(PerFrameBuffer);
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    HRESULT hr = g_device->CreateBuffer(&cbDesc, nullptr, &g_constantBuffer);
    if (FAILED(hr)) enabled = false;
}

NTSTATUS FrameHandler::AssignSwapChain(IDDCX_SWAPCHAIN hSwapChain, IDXGISwapChain* dxgiSwapChain) {
    if (!g_device || !g_deviceContext || !enabled) return STATUS_INVALID_PARAMETER;

    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = g_device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return STATUS_UNSUCCESSFUL;

    IDARG_IN_SWAPCHAINSETDEVICE swapChainArgs = {};
    swapChainArgs.pDevice = dxgiDevice;

    hr = IddCxSwapChainSetDevice(hSwapChain, &swapChainArgs);
    dxgiDevice->Release();
    if (FAILED(hr)) return STATUS_UNSUCCESSFUL;

    if (dxgiSwapChain) {
        ID3D11Texture2D* backBuffer = nullptr;
        hr = dxgiSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBuffer);
        if (SUCCEEDED(hr) && backBuffer) {
            hr = g_device->CreateRenderTargetView(backBuffer, nullptr, &g_outputRTV);
            backBuffer->Release();
            if (FAILED(hr)) return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}

void FrameHandler::ProcessFrame(IDDCX_SWAPCHAIN hSwapChain, LARGE_INTEGER PresentationTime) {
    if (!enabled || !g_deviceContext || !g_outputRTV) return;

    UNREFERENCED_PARAMETER(hSwapChain); // Silence C4100 warning

    /* Example: Use hSwapChain (simplified, adjust as per your IDDCX setup)
    IDARG_OUT_RELEASEANDACQUIREBUFFER bufferArgs = {};
    HRESULT hr = IddCxSwapChainReleaseAndAcquireBuffer(hSwapChain, &bufferArgs);
    if (FAILED(hr)) return;
    */

    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    g_deviceContext->ClearRenderTargetView(g_outputRTV, clearColor);

    if (g_vertexShader && g_pixelShader && g_inputLayout && g_constantBuffer) {
        g_deviceContext->IASetInputLayout(g_inputLayout);
        g_deviceContext->VSSetShader(g_vertexShader, nullptr, 0);
        g_deviceContext->PSSetShader(g_pixelShader, nullptr, 0);

        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT hr = g_deviceContext->Map(g_constantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
        if (SUCCEEDED(hr)) {
            PerFrameBuffer* bufferData = (PerFrameBuffer*)mappedResource.pData;
            bufferData->resolution = DirectX::XMFLOAT4(1920.0f, 1080.0f, 0.0f, 0.0f);
            bufferData->time = static_cast<float>(PresentationTime.QuadPart) / 10000000.0f;
            g_deviceContext->Unmap(g_constantBuffer, 0);
        }

        g_deviceContext->VSSetConstantBuffers(0, 1, &g_constantBuffer);
        g_deviceContext->PSSetConstantBuffers(0, 1, &g_constantBuffer);

        struct Vertex {
            DirectX::XMFLOAT3 pos;
            DirectX::XMFLOAT2 tex;
        };
        Vertex vertices[] = {
            {{ -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f }},
            {{  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f }},
            {{ -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f }},
            {{  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f }},
        };

        D3D11_BUFFER_DESC vbDesc = {};
        vbDesc.ByteWidth = sizeof(vertices);
        vbDesc.Usage = D3D11_USAGE_DEFAULT;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        D3D11_SUBRESOURCE_DATA vbData = {};
        vbData.pSysMem = vertices;
        ID3D11Buffer* vertexBuffer = nullptr;
        hr = g_device->CreateBuffer(&vbDesc, &vbData, &vertexBuffer);
        if (SUCCEEDED(hr)) {
            UINT stride = sizeof(Vertex);
            UINT offset = 0;
            g_deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &stride, &offset);
            g_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
            g_deviceContext->Draw(4, 0);
            vertexBuffer->Release();
        }

        g_deviceContext->OMSetRenderTargets(1, &g_outputRTV, nullptr);
    }
}