// TV_shader.h
#pragma once
#include <DirectXMath.h>

extern const int TVShader_VS_size;
extern const BYTE TVShader_VS[];

extern const int TVShader_PS_size;
extern const BYTE TVShader_PS[];

struct PerFrameBuffer
{
    DirectX::XMFLOAT4 resolution;
    float time;
    float padding[3]; // Align to 16 bytes
};