#include "Common.hlsl"
#include "PBRLighting.hlsl"
#include "lightInfo.hlsl"

#ifndef BLOCK_SIZE
#define BLOCK_SIZE 16// should be defined by the application.
#endif

Texture2D diffuseMap;
Texture2D metallicMap;
Texture2D roughnessMap;

TextureCube IrradianceMap;
TextureCube PrefilterMap[IBL_PREFILTER_ENVMAP_MIP_LEVEL];
Texture2D BrdfLUT2D;

// light grid 
//Texture2D<uint2> o_LightGrid;
//StructuredBuffer<uint> o_LightIndexList;

#define MAX_LIGHT_COUNT_IN_TILE 100
struct TileLightInfo
{
    uint LightIndices[MAX_LIGHT_COUNT_IN_TILE];
    uint LightCount;
};
StructuredBuffer<TileLightInfo> LightInfoList;

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
    float3x3 NormalMatrix = (float3x3) gInvTranModelMat;
    vout.normal = mul(vin.normal, NormalMatrix);
    
    return vout;
}

float DoAttenuation(float range, float d)
{
    return 1.0f - smoothstep(range * 0.75f, range, d);
}

float2 UVToScreen(float2 UVPos, float2 ScreenSize)
{
    return UVPos * ScreenSize;
}

float4 PSMain(PSInput pin) : SV_Target
{
#if 0
   
#elif 0
    const uint2 pos = uint2(pin.position.xy);
    uint2 tileIndex = uint2(floor(pos / BLOCK_SIZE));
    
    float c = (tileIndex.x + tileIndex.y) * 0.0001f;
    if (tileIndex.x % 2 == 0)
        c += 0.1f;
    if (tileIndex.y % 2 == 0)
        c += 0.1f;
    
    return float4((float3)c, 1.0f);
#elif 1
    uint startOffset, lightCount;


    //uint2 tileIndex = uint2(floor(pin.position.xy / BLOCK_SIZE));
    uint TileX = floor(pin.position.x / BLOCK_SIZE);
    uint TileY = floor(pin.position.y / BLOCK_SIZE);
    uint TileCountX = ceil(ScreenDimensions.x / BLOCK_SIZE);
    uint TileIndex = TileY * TileCountX + TileX;
    
    
    float3 albedo = diffuseMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float metallic = metallicMap.Sample(LinearWrapSampler, pin.tex).r;
    
    float3 N = normalize(pin.normal);
    float3 V = normalize(gEyePosW - pin.positionW);
    
    TileLightInfo LightInfo = LightInfoList[TileIndex];
    
    // lighting
    float3 totalDiffuse = float3(0.0, 0.0, 0.0);
    float3 totalSpecular= float3(0.0, 0.0, 0.0);

    for (int i = 0; i < LightInfo.LightCount; i++)
    {
        uint LightIndex = LightInfo.LightIndices[i];
        Light light = Lights[LightIndex];
    
        if (light.Type == 1) // directional light
        {
            float3 L = normalize(-light.DirectionW);
        
            float kd = saturate(dot(N, L));
            float3 lightDiffuse = light.Color.rgb * light.Intensity * kd;
        
            float3 H = normalize(V + L);
            float ks = pow(saturate(dot(N, H)), 32);
            float3 lightSpecular = light.Color.rgb * light.Intensity * ks;
        
            totalDiffuse += lightDiffuse;
            totalSpecular += lightSpecular;
        }
        else if (light.Type == 2) // point light
        {
            float3 L = light.PositionW.xyz - pin.positionW;
            float distance = length(L);
            L = L / distance;
            float attenuation = 1.0 / (distance * distance);
        
            float kd = saturate(dot(N, L));
            float3 lightDiffuse = light.Color.rgb * light.Intensity * kd;
        
            float3 H = normalize(V + L);
            float ks = pow(saturate(dot(N, H)), 32);
            float3 lightSpecular = light.Color.rgb * light.Intensity * ks;
        
            totalDiffuse += lightDiffuse * attenuation;
            totalSpecular += lightSpecular * attenuation;
        }
       
    }
    float3 diffuse = totalDiffuse * albedo;
    float3 specular = totalSpecular * metallic;
    
    float3 ambient = albedo * 0.05;
    
    float3 color = diffuse + specular + ambient;
    
    return float4(color, 1.0);
#endif
}