# Virtual Display Driver Shader Examples

This directory contains example HLSL shaders for the Virtual Display Driver (HDR), demonstrating different visual effects that can be applied to virtual displays.

## Shaders Included

### 1. Vortex Shader (`vortex_shader.h`)
- **Effect**: Split-screen effect with blue and red colored halves
- **Features**:
  - Left half: Blue with 40% opacity
  - Right half: Red with 40% opacity
  - Simple vertex/pixel shader pair for basic rendering
- **Usage**: Good for testing basic shader functionality and color blending

### 2. Wave Shader (`wave_shader.h`)
- **Effect**: Animated wave with horizontal bottom bar
- **Features**:
  - White horizontal bar at the bottom (3 pixels tall)
  - Animated sine wave that travels side-to-side
  - Wave amplitude controllable via PerFrameBuffer
  - Time-based animation
- **Usage**: Demonstrates animated effects and real-time parameter control

## File Structure

Each shader consists of two files:
- `.hlsl` - Source code (human-readable HLSL)
- `.h` - Compiled header (ready for Direct3D integration)

## Compilation Instructions

### HLSL Source Files
The `.hlsl` files can be compiled using the DirectX FX Compiler (fxc.exe):

```cmd
:: Compile vertex shader
fxc.exe /T vs_5_0 /E VSMain /Fo vortex_vs.cso vortex_shader.hlsl
fxc.exe /T vs_5_0 /E VSMain /Fo wave_vs.cso wave_shader.hlsl

:: Compile pixel shader
fxc.exe /T ps_5_0 /E PSMain /Fo vortex_ps.cso vortex_shader.hlsl
fxc.exe /T ps_5_0 /E PSMain /Fo wave_ps.cso wave_shader.hlsl
```

### Integration with Direct3D 11
The `.h` header files contain:
- Pre-compiled shader bytecode as byte arrays
- PerFrameBuffer structure definitions
- Proper DirectX::XMFLOAT4 alignment for constant buffers

### Usage Example
```cpp
#include "vortex_shader.h"

// Create vertex shader
ID3D11VertexShader* pVertexShader;
pDevice->CreateVertexShader(VortexVSBytecode, VortexVSBytecodeSize, nullptr, &pVertexShader);

// Create pixel shader
ID3D11PixelShader* pPixelShader;
pDevice->CreatePixelShader(VortexPSBytecode, VortexPSBytecodeSize, nullptr, &pPixelShader);

// Create constant buffer
D3D11_BUFFER_DESC cbDesc;
cbDesc.ByteWidth = sizeof(PerFrameBuffer);
cbDesc.Usage = D3D11_USAGE_DYNAMIC;
cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
// ... setup other desc fields

ID3D11Buffer* pConstantBuffer;
pDevice->CreateBuffer(&cbDesc, nullptr, &pConstantBuffer);

// Update constant buffer data
PerFrameBuffer frameData;
frameData.resolution = DirectX::XMFLOAT4(1920, 1080, 0, 0);
frameData.time = currentTime;
frameData.padding = 0.0f;
```

## PerFrameBuffer Structure

Both shaders use the same constant buffer structure:

```cpp
__declspec(align(16)) struct PerFrameBuffer
{
    DirectX::XMFLOAT4 resolution;  // Screen resolution (width, height, 0, 0)
    float time;                     // Time elapsed in seconds
    float amplitude;                // Wave amplitude (wave shader only)
    float padding;                  // Padding for 16-byte alignment
};
```

## Shader Profiles

- **Vertex Shaders**: vs_5_0 (DirectX 11 feature level)
- **Pixel Shaders**: ps_5_0 (DirectX 11 feature level)
- **Constant Buffers**: register(b0) binding

## Notes

- All shaders use fullscreen quad geometry (typically created from screen-sized triangle strip)
- Shader bytecode in headers is placeholder data - replace with actual compiled bytecode for production
- The PerFrameBuffer is aligned to 16 bytes for DirectX constant buffer requirements
- Both shaders are compatible with Direct3D 11 and later versions

## Integration Points

These shaders can be integrated into:
- SwapChainProcessor for real-time display effects
- Custom rendering pipelines
- Debug/visualization modes
- Performance testing scenarios

Each shader demonstrates different HLSL techniques and can serve as a foundation for more complex visual effects.