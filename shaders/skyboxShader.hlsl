
cbuffer objCBuffer : register(b0)
{
    float4x4 gModelMat;
    float4x4 gInvTranModelMat;
}

cbuffer passCBuffer : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    
    float3 gEyePosW;
    float gLightIndex;
    
    float3 gLightPos;
    float pad1;
    float3 gLightColor;
    float gIntensity;
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
    float4x4 View = gViewMat;
    View[3][0] = View[3][1] = View[3][2] = 0.0f;
    
    vout.position = mul(mul(float4(vin.position.xyz, 1.0), View), gProjMat).xyww;
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 N = normalize(pin.positionW);
    
    float3 color = CubeMap.SampleLevel(LinearWrapSampler, N, 0.0).rgb;
    
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}