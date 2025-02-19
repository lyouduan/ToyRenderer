#pragma once
#include "stdafx.h"
#include "D3D12Utils.h"

#define CSM_MAX_COUNT 4

enum class MeshType
{
    ModelMesh,
    Mesh,
};

enum class ShadowType
{
    NONE,
    PCSS,
    VSM,
    VSSM,
    ESM,
    EVSM,
    CSM,
    Count,
};

enum class GBufferType
{
    Position,
    Normal,
    Albedo,
    Specular,
    LightMap,
    Count,
};


// Constant Buffer
__declspec(align(16))
struct ObjCBuffer
{
    XMFLOAT4X4 ModelMat = MATH::IdentityMatrix;
    XMFLOAT4X4 InvTranModelMat = MATH::IdentityMatrix;
};

__declspec(align(16))
struct PassCBuffer
{
    XMFLOAT4X4 ViewMat = MATH::IdentityMatrix;
    XMFLOAT4X4 ProjMat = MATH::IdentityMatrix;
    XMFLOAT4X4 invProjMat = MATH::IdentityMatrix;
    XMFLOAT3 EyePosition = { 0.0, 0.0, 0.0 };
    FLOAT Pad0 = 0.0;

    XMFLOAT3 lightPos = { 0.0, 10.0, 25.0 };
    FLOAT Pad1 = 0.0;

    XMFLOAT3 lightColor = { 1.0, 1.0, 1.0 };
    FLOAT Intensity = 100.0;
};

__declspec(align(16))
struct MatCBuffer
{
    XMFLOAT4 DiffuseAlbedo = { 1.0, 1.0 ,1.0 ,1.0 };
    XMFLOAT3 FresnelR0 = { 0.5, 0.5 ,0.5 };
    float Roughness = 0.8;

    XMFLOAT4X4 MatTransform = MATH::IdentityMatrix;

    XMFLOAT3 EmissiveColor;
    float metallic = 0.5;
};


__declspec(align(16))
struct BlurCBuffer
{
    int gBlurRadius;

    // Support up to 11 blur weights.
    float w0;
    float w1;
    float w2;
    float w3;

    float w4;
    float w5;
    float w6;
    float w7;

    float w8;
    float w9;
    float w10;
};

__declspec(align(16))
struct CSMCBuffer
{
    XMFLOAT2 frustumVSFarZ[CSM_MAX_COUNT];
};

__declspec(align(16))
struct ScreenToViewParams
{
    XMFLOAT2 ScreenDimensions;
    XMFLOAT2 InvScreenDimensions;
};
// ===============No-Constant Buffer===============

struct LightInfo
{
    XMFLOAT4 PositionW;
    XMFLOAT4 DirectionW;
    XMFLOAT4 Color;

    float SpotlightAngle;
    float Range;
    float Intensity;
    float pad0;

    XMFLOAT4X4 ModelMat = MATH::IdentityMatrix;
    XMFLOAT4X4 ShadowTransform = MATH::IdentityMatrix;
    XMFLOAT4X4 CSMTransform[CSM_MAX_COUNT];

    int Type;
};
