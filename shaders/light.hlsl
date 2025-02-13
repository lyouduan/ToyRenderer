#include "Common.hlsl"
#include "lightInfo.hlsl"

Texture2D diffuseMap;
Texture2D specularMap;
Texture2D normalMap;

struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 tex : TEXCOORD;
};

struct PSInput
{
    float4 position : SV_Position;
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    
    vout.positionW = mul(float4(vin.position.xyz, 1.0f), gModelMat);
    
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vout.positionW, 1.0), viewProj);
    
    // uniform scale
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    return float4(Lights[gLightIndex].Color.xyz, 1.0);
}