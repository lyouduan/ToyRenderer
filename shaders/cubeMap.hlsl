
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
Texture2D map : register(t0);
Texture2D equirectangularMap : register(t1);

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
    float4x4 viewProj = mul(ViewMat, ProjMat);
    vout.position = mul(float4(vin.position.xyz, 1.0), viewProj);
    
    return vout;
}

#define PI 3.1415

float2 SphereToEquirectangular(float3 dir)
{
    float phi = atan2(dir.z, dir.x);
    float theta = acos(dir.y);
    
    float u = (phi + PI) / (2.0 * PI);
    float v = theta / PI;
    
    return float2(u, v);
}


float4 PSMain(PSInput pin) : SV_Target
{
    float2 uv = SphereToEquirectangular(normalize(pin.positionW));
    // dx坐标系在左上角， openGL在左下角
    //uv.y = 1.0 - uv.y;
    float3 color = float3(0, 0, 0);
    float a = map.Sample(LinearWrapSampler, uv).a;
    color = equirectangularMap.Sample(LinearWrapSampler, uv).rgb;
    return float4(color, a);
}