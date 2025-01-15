#pragma once
#include "stdafx.h"
#include "D3D12Utils.h"

enum class MeshType
{
    ModelMesh,
    Mesh,
};

__declspec(align(16))
struct ObjCBuffer
{
    XMFLOAT4X4 ModelMat = MATH::IdentityMatrix;
};

__declspec(align(16))
struct PassCBuffer
{
    XMFLOAT4X4 ViewMat = MATH::IdentityMatrix;
    XMFLOAT4X4 ProjMat = MATH::IdentityMatrix;
    XMFLOAT3 EyePosition = { 0.0, 0.0, 0.0 };
    FLOAT Pad0 = 0.0;

    XMFLOAT3 lightPos = { 0.0, 10.0, 25.0 };
    FLOAT Pad1 = 0.0;

    XMFLOAT3 lightColor = { 1500.0, 1500.0, 1500.0 };
    FLOAT Pad2 = 0.0;
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