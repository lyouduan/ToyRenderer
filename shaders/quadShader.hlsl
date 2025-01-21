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

PSInput VSMain(VSInput input)
{
    PSInput vout;
    vout.position = input.position;
    vout.tex = input.tex; 
    
    return vout;
}

Texture2D tex : register(t0);

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

float GetActualDepth(float depth, float zNear, float zFar)
{
    // 计算实际深度
    float actualDepth = zNear * zFar / (zFar - depth * (zFar - zNear));
    return actualDepth;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 color = tex.Sample(PointClampSampler, pin.tex).rgb;

    return float4(color, 1.0);
}