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

Texture2D tex;

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
    
    actualDepth = (actualDepth - zNear) / (zFar - zNear);
    return actualDepth;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float depth = tex.Sample(PointClampSampler, pin.tex).r;
    //float3 color = GetActualDepth(depth, 0.1, 1000);
    
    return float4(depth, depth, depth, 1.0);
}