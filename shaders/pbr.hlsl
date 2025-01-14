#include "PBRLighting.hlsl"


cbuffer objCBuffer : register(b0)
{
    float4x4 gModelMat;
}

cbuffer passCBuffer : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    
    float3 gEyePosW;
    float pad0;
    
    float3 gLightPos;
    float pad1;
    float3 gLightColor;
    float pad2;
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

Texture2D diffuseMap : register(t0);
Texture2D specularMap : register(t1);
Texture2D normalMap : register(t2);
TextureCube IrradianceMap : register(t0);

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
    float2 tex : TEXCOORD;
};

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    vout.positionW = mul(float4(vin.position.xyz, 1.0f), gModelMat);
    
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vout.positionW, 1.0), viewProj);
    
    vout.tex = vin.tex;
    
    // uniform scale
    float3x3 NormalMatrix = (float3x3) gModelMat;
    vout.normal = mul(vin.normal, NormalMatrix);
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 N = normalize(pin.normal);
    float3 V = normalize(gEyePosW - pin.positionW);
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    {
        float3 L = normalize(gLightPos - pin.positionW);
        
        // attenuation
        float distance = length(gLightPos - pin.positionW);
        float attenuation = 1.0 / (distance * distance);
        //float3 radiance = gLightColor * attenuation;
        float3 radiance = IrradianceMap.Sample(LinearWrapSampler, N).rgb;
        // cook-torrance brdf
        //float3 fr = DefaultBRDF(L, N, V, gRoughness, gMetallic, gDiffuseAlbedo.rgb);
        
        //float NdotL = saturate(dot(N, L));
        //Lo += fr * radiance * NdotL;
        Lo = DirectLighting(radiance, L, N, V, gRoughness, gMetallic, gDiffuseAlbedo.rgb, 1);

    }
    
    float3 ambient = 0.02 * gDiffuseAlbedo.rgb;
    
    float3 color = ambient + Lo;
    
    // HDR
    color = color /(color + float3(1.0, 1.0, 1.0));
    // gamma correct
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}