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
Texture2D normalMap;
Texture2D roughnessMap;

TextureCube IrradianceMap;
TextureCube PrefilterMap[IBL_PREFILTER_ENVMAP_MIP_LEVEL];
Texture2D BrdfLUT2D;

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
    vout.normal = mul(vin.normal, NormalMatrix);
    
        // 动态计算切线和副切线
    float3 P0 = vin.position; // 当前顶点位置
    float3 P1 = P0 + float3(1.0, 0.0, 0.0); // 邻近顶点位置（近似）
    float3 P2 = P0 + float3(0.0, 1.0, 0.0); // 邻近顶点位置（近似）

    float2 UV0 = vin.tex;
    float2 UV1 = UV0 + float2(1.0, 0.0); // UV 邻近点（近似）
    float2 UV2 = UV0 + float2(0.0, 1.0); // UV 邻近点（近似）

    float3 deltaPos1 = P1 - P0;
    float3 deltaPos2 = P2 - P0;

    float2 deltaUV1 = UV1 - UV0;
    float2 deltaUV2 = UV2 - UV0;

    float r = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
    float3 tangent = (deltaPos1 * deltaUV2.y - deltaPos2 * deltaUV1.y) * r;
    float3 bitangent = (deltaPos2 * deltaUV1.x - deltaPos1 * deltaUV2.x) * r;
    
    vout.tangent = tangent;
    vout.bitangent = bitangent;
    
    return vout;
}

float3 GetPrefilteredColor(float Roughness, float3 ReflectDir)
{
    float Level = Roughness * (IBL_PREFILTER_ENVMAP_MIP_LEVEL - 1);
    int FloorLevel = floor(Level);
    int CeilLevel = ceil(Level);

    float3 FloorSample = PrefilterMap[FloorLevel].SampleLevel(LinearWrapSampler, ReflectDir, 0).rgb;
    float3 CeilSample = PrefilterMap[CeilLevel].SampleLevel(LinearWrapSampler, ReflectDir, 0).rgb;
	
    float3 PrefilteredColor = lerp(FloorSample, CeilSample, (Level - FloorLevel));
    return PrefilteredColor;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3x3 TBN = float3x3(
        normalize(pin.tangent),
        normalize(pin.bitangent),
        normalize(pin.normal)
    );
    
    float3 N = normalMap.Sample(LinearWrapSampler, pin.tex).xyz;
    N = N * 2.0 - 1.0;
    N = normalize(mul(N, TBN));
    
    float3 V = normalize(gEyePosW - pin.positionW);
    float3 R = reflect(-V, N);
    
    float3 albedo = diffuseMap.Sample(LinearWrapSampler, pin.tex).rgb;
    albedo = pow(albedo, 2.2);
    float metallic = metallicMap.Sample(LinearWrapSampler, pin.tex).r;
    float roughness = roughnessMap.Sample(LinearWrapSampler, pin.tex).r;
    
    // lighting
    float3 Lo = float3(0.0, 0.0, 0.0);
    {
        float3 L = normalize(gLightPos - pin.positionW);
        // attenuation
        float distance = length(gLightPos - pin.positionW);
        float attenuation = max(1.0 / max(distance * distance, 0.01), 1 / max(gIntensity, 0.01));
        float3 radiance = gIntensity * gLightColor * attenuation;
       
        Lo += DirectLighting(radiance, L, N, V, roughness, metallic, albedo, 1);
    }
    
    // IBL ambient light
    float3 F0 = lerp(F0_DIELECTRIC.rrr, albedo, metallic);
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    
    float3 ambient = 0.0;
    {
        float3 irradiance = IrradianceMap.Sample(LinearWrapSampler, N).rgb;
        float3 diffuse = irradiance * albedo;
    
        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
        float3 prefilteredColor = GetPrefilteredColor(roughness, R);
        // LUT value
        float NdotV = saturate(dot(N, V));
        float2 brdf = BrdfLUT2D.Sample(LinearWrapSampler, float2(NdotV, roughness)).rg;
        float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        ambient = kD * diffuse + specular;
    }
    
    float ao = 0.02;
    float3 color = ambient + Lo;
    
    // HDR
    color = color / (color + float3(1.0, 1.0, 1.0));
    // gamma correct
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}