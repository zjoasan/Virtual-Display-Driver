# Shader/iGPU Integration Unified Diffs

This directory contains three unified diff files that document the integration of shader support and iGPU detection capabilities from the testcode branch into the main codebase.

## Diff Files

### 1. `AdapterOption_h_shader_igpu.diff`
**Files compared:** `testcode/AdapterOption.h` vs `Common/Include/AdapterOption.h`

**Summary:** This diff shows the complete addition of iGPU detection and support functionality to the AdapterOption class. Since this is a NEW FILE in testcode (doesn't exist in Common/Include yet), the diff shows all new code as additions.

**Key additions:**
- `findiGPU()` - Function to detect integrated GPU from available adapters using:
  - Vendor ID detection (Intel 0x8086, AMD 0x1002)
  - Dedicated video memory threshold (< 512MB)
  - GPU naming patterns (Intel HD/UHD)
  - Fallback to lowest memory GPU
- `getiGPUName()` - Helper function to retrieve iGPU name
- AdapterOption class members:
  - `bool hasiGPU` - Flag indicating iGPU detection
  - `LUID iGPULuid` - iGPU's unique identifier
  - `wstring iGPU_name` - iGPU friendly name
- AdapterOption class methods:
  - `selectiGPU()` - Detect and select iGPU
  - `getiGPULuid()` - Retrieve iGPU LUID
  - `hasIntegratedGPU()` - Check if iGPU available
  - `getiGPUName()` - Get iGPU name

**Comment formatting:** All iGPU-related code is marked with `// #--- iGPU SUPPORT:` prefixes

---

### 2. `Driver_h_shader_igpu.diff`
**Files compared:** `testcode/Driver.h` vs `Virtual Display Driver (HDR)/MttVDD/Driver.h`

**Summary:** This diff shows the addition of shader support member variables to the Direct3DDevice struct. These members are currently commented out as placeholders for future shader implementation.

**Key additions (lines 67-74):**
- Commented-out ComPtr members for shader support:
  - `ID3D11VertexShader VertexShader` - Vertex shader interface
  - `ID3D11PixelShader PixelShader` - Pixel shader interface
  - `ID3D11Buffer PerFrameBuffer` - Constant buffer for shader parameters

**Comment formatting:** All shader-related code is marked with `// #--- SHADER SUPPORT:` prefixes

---

### 3. `Driver_cpp_shader_igpu.diff`
**Files compared:** `testcode/Driver.cpp` vs `Virtual Display Driver (HDR)/MttVDD/Driver.cpp`

**Summary:** This diff shows the addition of global shader support variables and multi-GPU rendering configuration settings.

**Key additions:**

**Lines 118-134:**
- Shader bytecode storage variables:
  - `g_VertexShaderBytecode` - Compiled vertex shader bytecode
  - `g_PixelShaderBytecode` - Compiled pixel shader bytecode
  - `g_ShaderHeaderPath` - Path to shader header file
  - `g_ShaderEnabled` - Master enable flag for shader rendering

- Multi-GPU shader rendering variables (mostly commented):
  - `g_UseiGPUForShaders` - Enable iGPU for shader processing while displaying on dGPU
  - `g_iGPULuid` - iGPU LUID for shader processing
  - `g_iGPUDevice` - iGPU Direct3DDevice for shaders
  - `g_StagingTexture` - Cross-adapter staging texture
  - `g_iGPURenderTarget` - iGPU render target for shader output

**Lines 217-219 (SettingsQueryMap):**
- Added shader configuration entry:
  - `{L"ShaderRendererEnabled", {L"SHADERRENDERER", L"shader_renderer"}}`

**Comment formatting:**
- Shader-related: `// #--- SHADER SUPPORT: [description]`
- iGPU-related: `// #--- iGPU SUPPORT: [description]`

---

## Applying the Diffs

To apply these diffs to your main codebase, use git apply:

```bash
git apply AdapterOption_h_shader_igpu.diff
git apply Driver_h_shader_igpu.diff
git apply Driver_cpp_shader_igpu.diff
```

Or use standard patch command:

```bash
patch -p1 < AdapterOption_h_shader_igpu.diff
patch -p1 < Driver_h_shader_igpu.diff
patch -p1 < Driver_cpp_shader_igpu.diff
```

---

## Integration Notes

### Shader Support Architecture

The shader support is designed to:
1. Load compiled HLSL bytecode from header files
2. Create DirectX 11 shader objects during device initialization
3. Apply shaders during swap chain processing (frame rendering)
4. Support custom shader effects (vortex, wave, etc.)

Current shaders available:
- `vortex_shader.hlsl` - Split screen shader with blue (left) and red (right) at 40% opacity
- `wave_shader.hlsl` - Animated wave shader with bottom white bar and sine wave animation

### Multi-GPU iGPU Rendering Strategy

The iGPU support enables a hybrid rendering approach:
1. Display on selected discrete GPU (dGPU) for high-performance output
2. Use integrated GPU (iGPU) for shader processing to offload dGPU
3. Transfer rendered frames between GPUs using staging textures
4. Enables shader effects on systems with limited dGPU resources

### Configuration

Shader/iGPU features can be configured via:
- Named pipe commands (real-time control)
- XML configuration file (`vdd_settings.xml`)
- Registry settings (persistent configuration)

### Status

- **iGPU Detection:** ✅ Fully implemented and functional
- **Shader Infrastructure:** 🔄 Placeholder/ready for implementation
- **Multi-GPU Rendering:** 🔄 Commented out, ready for implementation
- **Shader Processing:** ❌ Not yet implemented in SwapChainProcessor

---

## Next Steps for Full Integration

1. **Uncomment and implement shader member variables in Driver.h**
2. **Implement shader loading and compilation in Driver.cpp**
3. **Add shader application logic in SwapChainProcessor::RunCore()**
4. **Implement cross-adapter texture copying for multi-GPU rendering**
5. **Add shader parameter binding and constant buffer updates**
6. **Test with available shaders (vortex_shader, wave_shader)**
7. **Performance test multi-GPU rendering pipeline**

---

## Testing Recommendations

Before integration:
1. Verify iGPU detection accuracy on various hardware configurations
2. Test with Intel HD/UHD, AMD APU, and discrete GPU systems
3. Validate LUID resolution for multi-GPU systems
4. Test shader loading and compilation
5. Verify cross-adapter texture sharing compatibility

---

## Contact

For questions or issues related to these diffs, please refer to the main project repository or contact the development team.
