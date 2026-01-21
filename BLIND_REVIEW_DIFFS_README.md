# Blind Review Diffs - Shader/iGPU Integration

## Overview
Three unified diff files have been generated to show the integration of shader and iGPU-related code into the Virtual Display Driver. All shader/iGPU code is prefixed with `// #--- ` for blind review purposes.

## Critical Formatting Rules Applied

1. **Code lines**: `// #--- [code]` (e.g., `// #--- int x = 0;`)
2. **Remarks/comments**: `// #--- // [text]` (e.g., `// #--- // iGPU detection function`)
3. **Later activation**: Remove `// #--- ` globally to activate all code while preserving comments

## Generated Diff Files

### 1. AdapterOption.h.blind.diff
**Location**: `/Common/Include/AdapterOption.h`

**Added iGPU functionality** (all prefixed with `// #--- `):
- `findiGPU()` - Standalone function to find integrated GPU (lines 68-142 in testcode)
  - Detects iGPU by vendor ID (Intel 0x8086, AMD 0x1002)
  - Memory threshold-based detection (< 512MB)
  - Intel HD/UHD graphics name pattern matching
- `getiGPUName()` - Standalone function to get iGPU name (lines 144-160 in testcode)
- **Class member variables**:
  - `bool hasiGPU` - Indicates if iGPU was detected
  - `LUID iGPULuid` - Integrated GPU's unique identifier
  - `wstring iGPU_name` - Integrated GPU name
- **Class methods**:
  - `selectiGPU()` - Detect and select the iGPU, returns true if found
  - `getiGPULuid()` - Get the iGPU LUID if available
  - `hasIntegratedGPU()` - Check if iGPU is available
  - `getiGPUName()` - Get iGPU name

**Note**: `ResolveAdapterLuidFromPciBus()` was NOT included as it already exists in the destination file (lines 70-128).

### 2. Driver.h.blind.diff
**Location**: `/Virtual Display Driver (HDR)/MttVDD/Driver.h`

**Added shader support members** in `Direct3DDevice` struct (all prefixed with `// #--- `):
- `Microsoft::WRL::ComPtr<ID3D11VertexShader> VertexShader` - Vertex shader for frame processing
- `Microsoft::WRL::ComPtr<ID3D11PixelShader> PixelShader` - Pixel shader for frame processing
- `Microsoft::WRL::ComPtr<ID3D11Buffer> PerFrameBuffer` - Optional constant buffers for shader parameters
- All shader members are commented out (double-commented) to show they're optional/planned additions

### 3. Driver.cpp.blind.diff
**Location**: `/Virtual Display Driver (HDR)/MttVDD/Driver.cpp`

**Added shader/iGPU global variables** (all prefixed with `// #--- `):
- `std::vector<BYTE> g_VertexShaderBytecode` - Global vertex shader bytecode storage
- `std::vector<BYTE> g_PixelShaderBytecode` - Global pixel shader bytecode storage
- `wstring g_ShaderHeaderPath` - Path to shader header files
- `bool g_ShaderEnabled` - Flag to enable/disable shader rendering

**Added multi-GPU shader variables** (all commented out and prefixed with `// #--- `):
- `bool g_UseiGPUForShaders` - Enable iGPU for shader processing while displaying on dGPU
- `LUID g_iGPULuid` - iGPU LUID for shader processing (commented out)
- `std::shared_ptr<Direct3DDevice> g_iGPUDevice` - iGPU Direct3DDevice for shaders (commented out)
- `Microsoft::WRL::ComPtr<ID3D11Texture2D> g_StagingTexture` - Cross-adapter staging texture (commented out)
- `Microsoft::WRL::ComPtr<ID3D11RenderTargetView> g_iGPURenderTarget` - iGPU render target (commented out)

**Added SettingsQueryMap entry**:
- `{L"ShaderRendererEnabled", {L"SHADERRENDERER", L"shader_renderer"}}` - Settings query for shader renderer

## Integration Workflow

### Current State (Blind Review)
All shader/iGPU code is disabled and marked with `// #--- ` prefix:
```cpp
// #--- bool g_ShaderEnabled = false;
// #--- // iGPU detection function
```

### Activation Workflow
1. **Manual Review**: Review all diffs to identify conflicts with existing code
2. **Resolve Conflicts**: Manually merge any overlapping functionality
3. **Global Activation**: Use find-and-replace to remove `// #--- ` prefix:
   - Find: `// #--- `
   - Replace: (empty)
4. **Result**: All code activates while preserving comments:
   ```cpp
   bool g_ShaderEnabled = false;
   // iGPU detection function
   ```

## Key Observations

### AdapterOption.h
- The testcode version includes comprehensive iGPU detection logic not present in the current version
- No conflicts with `ResolveAdapterLuidFromPciBus()` as it already exists in the current file
- Class members can be safely added without breaking existing functionality

### Driver.h
- Shader support members are additive - no existing code conflicts
- All shader members are commented out, showing they're planned additions
- Integration requires uncommenting during activation

### Driver.cpp
- Shader variables are new additions after line 116 (`wstring ColourFormat`)
- Multi-GPU shader variables are mostly commented out (double-commented)
- SettingsQueryMap entry fits between "Colour End" and "EDID Integration Begin" sections
- No conflicts with existing variables

## Next Steps

1. **Apply diffs manually** to review integration points
2. **Test for conflicts** with existing code patterns
3. **Verify compatibility** with current driver architecture
4. **Global find-replace** to activate: Remove `// #--- ` prefix
5. **Compile and test** to ensure shader/iGPU functionality works correctly

## File Locations
- `AdapterOption.h.blind.diff` - iGPU detection and selection functionality
- `Driver.h.blind.diff` - Shader support members for Direct3DDevice
- `Driver.cpp.blind.diff` - Global shader/iGPU variables and settings

All diffs use unified format with context lines for clear integration mapping.
