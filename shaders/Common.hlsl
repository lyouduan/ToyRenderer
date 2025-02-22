#ifndef COMMON_H
#define COMMON_H

#include "Sampler.hlsl"

cbuffer objCBuffer
{
    float4x4 gModelMat;
    float4x4 gPreModelMat;
    float4x4 gInvTranModelMat;
}

cbuffer passCBuffer
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    float4x4 gInvProjMat;
    float4x4 gPreViewProjMat;
    
    float2 ScreenDimensions;
    float2 InvScreenDimensions;
    
    float3 gEyePosW;
    float gLightIndex;
    
    float3 gLightPos;
    float gRoughness;
    float3 gLightColor;
    float gIntensity;
}

cbuffer matCBuffer
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float Roughness;
    float4x4 gMatTransform;
    
    float3 gEmissvieColor;
    float gMetallic;
}

#endif 