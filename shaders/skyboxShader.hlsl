
cbuffer objCBuffer : register(b0)
{
    float4x4 ModelMat;
}

cbuffer passCBuffer : register(b1)
{
    float4x4 ViewMat;
    float4x4 ProjMat;
    float3 EyePosW;
    float padding;
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

TextureCube CubeMap : register(t0);

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    
    // Use local vertex position as cubemap lookup vector.
    vout.positionW = vin.position.xyz;
    
    //float4 posW = mul(float4(vin.position.xyz, 1.0f), ModelMat);
    
    // Remove translation from the view matrix
    float4x4 View = ViewMat;
    View[3][0] = View[3][1] = View[3][2] = 0.0f;
    
    vout.position = mul(mul(float4(vin.position.xyz, 1.0), View), ProjMat).xyww;
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    return float4(CubeMap.SampleLevel(LinearWrapSampler, pin.positionW, 0.0).rgb, 1.0);
}