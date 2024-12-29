
cbuffer MVPcBuffer : register(b0)
{
    matrix ModelMat;
    matrix ViewProjMat;
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

Texture2D diffuseMap : register(t0);

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    vout.positionW = mul(ModelMat, float4(vin.position.xyz, 1.0f));
    vout.position = mul(ViewProjMat, float4(vout.positionW, 1.0f));
    vout.tex = vin.tex;
    vout.normal = vin.normal;
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    return float4(0.5, 0.5, 0.5, 1.0);
}