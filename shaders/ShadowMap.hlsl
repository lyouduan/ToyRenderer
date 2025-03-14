#include "Common.hlsl"
#include "Shadows.hlsl"
#include "PBRLighting.hlsl"
#include "lightInfo.hlsl"

Texture2D diffuseMap;
Texture2D metallicMap;
Texture2D roughnessMap;

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
    float3 positionW : POSITION0;
    float Depth : DEPTH;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD0;
    float3 tangent : TEXCOORD1;
    float3 bitangent : TEXCOORD2;
};

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    vout.positionW = mul(float4(vin.position.xyz, 1.0f), gModelMat);

#if USE_CSM 
    float4x4 MV = mul(gModelMat, gViewMat);
    vout.Depth = mul(float4(vin.position.xyz, 1.0), MV).z;
#endif
    
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vout.positionW, 1.0), viewProj);
    
    vout.tex = vin.tex;
    
    // uniform scale
    float3x3 NormalMatrix = (float3x3) gModelMat;
    vout.normal = mul(vin.normal, (float3x3) gModelMat);
    //vout.normal = vin.normal;
    
    return vout;
}

float DoAttenuation(float range, float d)
{
    return 1.0f - smoothstep(range * 0.75f, range, d);
}

float ComputeShadowBias(float3 normal, float3 lightDir, float3 viewDir)
{
    float NdotL = max(dot(normal, lightDir), 0.0);
    float NdotV = max(dot(normal, viewDir), 0.0);
    
    // 计算 Slope-Scale Bias
    float bias = 0.01 * tan(acos(NdotL)) * max(0.05, 1.0 - NdotV);
    
    // 限制最大偏移，防止阴影断裂
    return clamp(bias, 0.01, 0.05);
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 albedo = diffuseMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float metallic = metallicMap.Sample(LinearWrapSampler, pin.tex).r;
    if (metallic == 0.0)
        metallic = 0.1;
    float3 N = normalize(pin.normal);
    float3 V = normalize(gEyePosW - pin.positionW);
    
    // lighting
    float3 totalDiffuse = float3(0.0, 0.0, 0.0);
    float3 totalSpecular = float3(0.0, 0.0, 0.0);
    
    float4 CascadedArea = float4(0.0, 0.0, 0.0,0.0f);
    
    for (int i = 0; i < 1; i++)
    {
        Light light = Lights[i];
    
        float3 L = normalize(-light.DirectionW.xyz);
        
        float kd = saturate(dot(N, L));
        float3 lightDiffuse = light.Color.rgb * light.Intensity * kd;
        
        float3 H = normalize(V + L);
        float ks = pow(saturate(dot(N, H)), 32);
        float3 lightSpecular = light.Color.rgb * light.Intensity * ks;
        
        float4 ShadowPos = mul(float4(pin.positionW, 1.0), light.ShadowTransform);
        
        float ShadowFactor = 1.0;
        
        float bias = ComputeShadowBias(N, L, V);
#if SHADOW_MAPPING       
        ShadowFactor = CalcVisibility(ShadowPos, bias);
#elif USE_CSM
        int cascadeIdx = -1;
         for (int i = 0; i < CSM_MAX_COUNT; ++i)
        {
            if (pin.Depth < frustumVSFarZ[i].y)
            {
                cascadeIdx = i;
                break;
            }
        }
        
        if(cascadeIdx == 0) 
        {
        CascadedArea = float4(1.0, 0.0, 0.0,1.0);
        }
        else if(cascadeIdx == 1) 
        {
            CascadedArea = float4(0.0, 1.0, 0.0,1.0);
        }
        else if(cascadeIdx == 2) 
        {
            CascadedArea = float4(0.0, 0.0, 1.0,1.0);
        }
        else
        {
            CascadedArea = float4(1.0, 0.0, 1.0,1.0);
        }
        
        if (cascadeIdx != -1)
        {
            ShadowPos = mul(float4(pin.positionW, 1.0), light.CSMTransform[cascadeIdx]);
            ShadowFactor = CSM(ShadowPos, cascadeIdx);
        }
#endif
        totalDiffuse += lightDiffuse * ShadowFactor;
        totalSpecular += lightSpecular * ShadowFactor;
    }
    
    float3 diffuse = totalDiffuse * albedo;
    float3 specular = totalSpecular * metallic;
    
    float3 ambient = albedo * 0.2;
    
    float3 color = diffuse + specular + ambient;
    
    return float4(lerp(color, CascadedArea.xyz, 0.0f), 1.0);
}