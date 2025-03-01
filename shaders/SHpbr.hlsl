#include "Common.hlsl"
#include "PBRLighting.hlsl"

TextureCube IrradianceMap;
TextureCube PrefilterMap[IBL_PREFILTER_ENVMAP_MIP_LEVEL];
Texture2D BrdfLUT2D;
StructuredBuffer<float3> SH_Coefficients;

float GetSHBasis(int i, float3 N)
{
    switch (i)
    {
        case 0:
            return 0.28209479f;
        case 1:
            return 0.48860251f * N.y;
        case 2:
            return 0.48860251f * N.z;
        case 3:
            return 0.48860251f * N.x;
        case 4:
            return 1.09254843f * N.x * N.y;
        case 5:
            return 1.09254843f * N.y * N.z;
        case 6:
            return 0.31539157f * (-N.x * N.x - N.y * N.y + 2 * N.z * N.z);
        case 7:
            return 1.09254843f * (N.z * N.x);
        case 8:
            return 0.54627421f * (N.x * N.x - N.y * N.y);
        default:
            return 0.0f;
    }
}

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
    float3 N = normalize(pin.normal);
    float3 V = normalize(gEyePosW - pin.positionW);
    float3 R = reflect(-V, N);
    
    float3 Lo = float3(0.0, 0.0, 0.0);
    {
        float3 L = gLightPos - pin.positionW;
        float distance = length(L);
        L = L / distance;
        
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = gIntensity * gLightColor * attenuation;
        // cook-torrance brdf
        //float3 fr = DefaultBRDF(L, N, V, gRoughness, gMetallic, gDiffuseAlbedo.rgb);
        
        //float NdotL = saturate(dot(N, L));
        //Lo += fr * radiance * NdotL;
        Lo = DirectLighting(radiance, L, N, V, Roughness, gMetallic, gDiffuseAlbedo.rgb, 1);
    }
    
    // IBL ambient light
    float3 ambient = 0.0;
    {
        float3 F0 = lerp(F0_DIELECTRIC.rrr, gDiffuseAlbedo.rgb, gMetallic);
        float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, Roughness);
        float3 kS = F;
        float3 kD = 1.0 - kS;
        kD *= 1.0 - gMetallic;
        
        float3 irradiance = IrradianceMap.Sample(LinearWrapSampler, N).rgb;
        float3 diffuse = irradiance * gDiffuseAlbedo.rgb;
    
        // sample both the pre-filter map and the BRDF lut and combine them together as per the Split-Sum approximation to get the IBL specular part.
        float3 prefilteredColor = GetPrefilteredColor(Roughness, R);
    
        // LUT value
        float NdotV = saturate(dot(N, V));
        float2 brdf = BrdfLUT2D.Sample(LinearWrapSampler, float2(NdotV, Roughness)).rg;
        float3 specular = prefilteredColor * (F * brdf.x + brdf.y);
        ambient = kD * diffuse + specular;
    }
    
    float3 color = ambient + Lo;
    
    // HDR
    color = color / (color + float3(1.0, 1.0, 1.0));
    // gamma correct
    color = pow(color, 1.0 / 2.2);
    
    float3 diffuse = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < 9; i++)
    {
        diffuse += SH_Coefficients[i] * GetSHBasis(i, N);
    }
    
    
    return float4(diffuse, 1.0);
}