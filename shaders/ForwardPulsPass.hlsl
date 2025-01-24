#include "PBRLighting.hlsl"
#include "lightInfo.hlsl"
#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16// should be defined by the application.
#endif
cbuffer objCBuffer : register(b0)
{
    float4x4 gModelMat;
    float4x4 gInvTranModelMat;
}

cbuffer passCBuffer : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    
    float3 gEyePosW;
    float gLightIndex;
    
    float3 gLightPos;
    float pad1;
    float3 gLightColor;
    float gIntensity;
}

cbuffer matCBuffer : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
    
    float3 gEmissvieColor;
    float gMetallic;
}


Texture2D diffuseMap;
Texture2D metallicMap;
Texture2D roughnessMap;

TextureCube IrradianceMap;
TextureCube PrefilterMap[IBL_PREFILTER_ENVMAP_MIP_LEVEL];
Texture2D BrdfLUT2D;

// light grid 
Texture2D<uint2> o_LightGrid;
StructuredBuffer<uint> o_LightIndexList;

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

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
};

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    vout.positionW = mul(float4(vin.position.xyz, 1.0f), gModelMat);
    
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vout.positionW, 1.0), viewProj);
    
    vout.tex = vin.tex;
    
    // uniform scale
    float3x3 NormalMatrix = (float3x3)gModelMat;
    vout.normal = mul(vin.normal, (float3x3) gModelMat);
    //vout.normal = vin.normal;
    
    return vout;
}

float DoAttenuation(float range, float d)
{
    return 1.0f - smoothstep(range * 0.75f, range, d);
}

float4 PSMain(PSInput pin) : SV_Target
{
    uint startOffset, lightCount;

    uint2 tileIndex = uint2(floor(pin.position.xy / BLOCK_SIZE));
    
    startOffset = o_LightGrid[tileIndex].x;
    lightCount = o_LightGrid[tileIndex].y;
    
    float3 albedo = diffuseMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float metallic = metallicMap.Sample(LinearWrapSampler, pin.tex).r;
    if (metallic == 0.0)
        metallic = 0.1;
    float3 N = normalize(pin.normal);
    float3 V = normalize(gEyePosW - pin.positionW);
    
    // lighting
    float3 totalDiffuse = float3(0.0, 0.0, 0.0);
    float3 totalSpecular= float3(0.0, 0.0, 0.0);
    for (int i = 0; i < lightCount; i++)
    {
        int lihgtIndex = o_LightIndexList[startOffset + i];
        
        Light light = Lights[lihgtIndex];
    
        float3 L = light.PositionW.xyz - pin.positionW;
        float distance = length(L);
        L = L / distance;
        float attenuation = 1.0 / (distance * distance);
        
        float kd = saturate(dot(N, L));
        float3 lightDiffuse = light.Color.rgb * light.Intensity * kd;
        
        float3 H = normalize(V + L);
        float ks = pow(saturate(dot(N, H)), 32);
        float3 lightSpecular = light.Color.rgb * light.Intensity * ks;
        
        totalDiffuse  += lightDiffuse  * attenuation;
        totalSpecular += lightSpecular * attenuation;
    }
    
    float3 diffuse = totalDiffuse * albedo;
    float3 specular = totalSpecular * metallic;
    
    float3 ambient = albedo * 0.5;
    
    float3 color = diffuse + specular * ambient;
    
    return float4(color, 1.0);
}