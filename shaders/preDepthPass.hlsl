#include "Common.hlsl"

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
    float2 tex : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
#if USE_CSMINST   
    uint layerIndex : SV_RenderTargetArrayIndex;
#endif

};

struct passCB
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
};

StructuredBuffer<passCB> InstancePassCB;

PSInput VSMain(VSInput vin, uint instanceID : SV_InstanceID)
{
    PSInput vout;
    vout.positionW = mul(float4(vin.position.xyz, 1.0f), gModelMat);
    
#if USE_CSMINST
    passCB instPassCB = InstancePassCB[instanceID];
    float4x4 viewProj = mul(instPassCB.gViewMat, instPassCB.gProjMat);
    vout.layerIndex = instanceID;
#else  
    float4x4 viewProj = mul(gViewMat, gProjMat);
#endif

    vout.position = mul(float4(vout.positionW, 1.0), viewProj);
    
    return vout;
}

void PSMain(PSInput pin)
{
    // return depth value
}