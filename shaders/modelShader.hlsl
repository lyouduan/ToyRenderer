
cbuffer objCBuffer : register(b0)
{
    float4x4 ModelMat;
}

cbuffer passCBuffer : register(b1)
{
    float4x4 ViewMat;
    float4x4 ProjMat;
    float3 gEyePosW;
    float pad0;
}

Texture2D diffuseMap : register(t0);
Texture2D specularMap : register(t1);
Texture2D normalMap : register(t2);

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
    vout.positionW = mul(float4(vin.position.xyz, 1.0f), ModelMat);
    vout.position = mul(mul(float4(vout.positionW, 1.0f), ViewMat), ProjMat);
    vout.tex = vin.tex;
    vout.normal = vin.normal;
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 lightPos = float3(0, 200.0, -50.0);
    
    float3 eyePos = float3(0.0, 0.0, 1.0);
    
    float3 diffuse = diffuseMap.Sample(LinearWrapSampler, pin.tex).xyz;
    //float3 specular = specularMap.Sample(LinearWrapSampler, pin.tex).xyz;
    //float3 normal = normalMap.Sample(LinearWrapSampler, pin.tex).xyz * 2.0 - 1.0;
    pin.normal = normalize(pin.normal);
    
    float3 lightDir = normalize(lightPos - pin.positionW);
    //float3 eyeDir = normalize(eyePos - pin.positionW);
    
    float NdotL = max(dot(lightDir, pin.normal), 0.0);
    
    //float3 reflected = reflect(lightDir, pin.normal);
    //float ks = max(dot(reflected, eyeDir), 0.0);
    
    float3 ambient = diffuse * 0.2;
    
    float3 color = diffuse * NdotL + ambient;
    
    return float4(color, 1.0);
}