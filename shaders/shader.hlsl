
cbuffer MVPcBuffer : register(b0)
{
    matrix MVP;
}

Texture2D diffuseMap : register(t0);
Texture2D normalMap : register(t1);

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
    float2 tex : TEXCOORD;
};

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    vout.position = mul(MVP, float4(vin.position.xyz, 1.0f));
    
    vout.tex = vin.tex;
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 color = diffuseMap.Sample(LinearWrapSampler, pin.tex).xyz;
    float4 a = float4(color, 1.0);
    
    float3 normal = normalMap.Sample(LinearWrapSampler, pin.tex).xyz;
    
    return float4(normal, a.z);
}