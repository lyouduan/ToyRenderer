
cbuffer MVPcBuffer : register(b0)
{
    matrix MVP;
}

Texture2D diffuseMap : register(t0);

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

struct PSInput
{
    float4 position : SV_Position;
    float2 tex : TEXCOORD;
};

PSInput VSMain(float4 position : POSITION, float2 tex : TEXCOORD)
{
    PSInput result;
    result.position = mul(MVP, float4(position.xyz, 1.0f));
    
    result.tex = float2(tex.x, tex.y);
    
    return result;
}

float4 PSMain(PSInput input) : SV_Target
{
    float3 color = diffuseMap.Sample(LinearWrapSampler, input.tex).xyz;
    return float4(color, 1.0);
}