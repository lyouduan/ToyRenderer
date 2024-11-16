
cbuffer ModelViewProjectionCB : register(b0)
{
    matrix MVP;
}

Texture2D tex : register(t0);
SamplerState LinearSampler : register(s0);

struct PSInput
{
    float4 position : SV_Position;
    float2 tex : TEXCOORD;
};

PSInput VSMain(float4 position : POSITION, float2 tex : TEXCOORD)
{
    PSInput result;
    result.position = mul(MVP, float4(position.xyz, 1.0f));
    result.tex = tex;
    
    return result;
}

float4 PSMain(PSInput input) : SV_Target
{
    float3 color = tex.Sample(LinearSampler, input.tex).xyz;
    return float4(color, 1.0);
}