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

#define PI 3.14159265359
static const float F0_DIELECTRIC = 0.04f;

float pow4(float n)
{
    return n * n * n * n;
}

float pow5(float n)
{
    return n * n * n * n * n;
}

// Lambert diffuse
float3 LambertDiffuse(float3 DiffuseColor)
{
    return DiffuseColor * (1 / PI);
}

// GGX (Trowbridge-Reitz)
float GGX(float a2, float NoH)
{
    float NoH2 = NoH * NoH;
    float d = NoH2 * (a2 - 1.0f) + 1.0f;

    return a2 / (PI * d * d);
}

// Fresnel, Schlick approx.
float3 FresnelSchlick(float3 F0, float VoH)
{
    return F0 + (1 - F0) * exp2((-5.55473 * VoH - 6.98316) * VoH);
}

// Schlick-GGX
float GeometrySchlickGGX(float Roughness, float NoV)
{
    float k = pow(Roughness + 1, 2) / 8.0f;

    return NoV / (NoV * (1 - k) + k);
}

// Cook-Torrance Specular
float3 SpecularGGX(float3 N, float3 L, float3 V, float Roughness, float3 F0)
{
    float3 H = normalize(V + L);
    
    float NoL = saturate(dot(N, L));
    float NoV = saturate(dot(N, V));
    float VoH = saturate(dot(V, H));
    float NoH = saturate(dot(N, H));

    float a2 = pow4(Roughness);
    float D = GGX(a2, NoH);
    float3 F = FresnelSchlick(F0, VoH);
    float G = GeometrySchlickGGX(Roughness, NoV) * GeometrySchlickGGX(Roughness, NoL);

    return (D * G * F) / (4 * max(NoL * NoV, 0.001f)); // 0.01 is added to prevent division by 0
}

float3 DefaultBRDF(float3 LightDir, float3 Normal, float3 ViewDir, float Roughness, float Metallic, float3 BaseColor)
{
    float3 F0 = lerp(F0_DIELECTRIC.rrr, BaseColor.rgb, Metallic);

    // Base color remapping
    float3 DiffuseColor = (1.0 - Metallic) * BaseColor; // Metallic surfaces have no diffuse reflections
    
    float3 DiffuseBRDF = LambertDiffuse(DiffuseColor);
    float3 SpecularBRDF = SpecularGGX(Normal, LightDir, ViewDir, Roughness, F0);
    
    float3 H = normalize(ViewDir + LightDir);
    
    float VoH = saturate(dot(ViewDir, H));
    
    float3 ks = FresnelSchlick(F0, VoH);
    float3 kd = 1 - ks;
    return DiffuseBRDF + SpecularBRDF;
}

float3 DirectLighting(float3 Radiance, float3 LightDir, float3 Normal, float3 ViewDir, float Roughness, float Metallic, float3 BaseColor, float ShadowFactor)
{
    float3 BRDF = DefaultBRDF(LightDir, Normal, ViewDir, Roughness, Metallic, BaseColor);
    
    float NoL = saturate(dot(Normal, LightDir));
    
    return Radiance * BRDF * NoL * ShadowFactor;
}

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