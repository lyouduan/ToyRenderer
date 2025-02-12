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

Texture2D equirectangularMap;

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
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vin.position.xyz, 1.0), viewProj);
    
    return vout;
}

#define PI 3.1415

float2 SphereToEquirectangular(float3 dir)
{
    // phi = arctan2(z,w)
    // theta = arccos(y)
    // 将phi和theta表示成2D贴图
    // phi   = [-PI, PI]
    // theta = [0, PI]
    // u = (phi + PI) / (2PI)
    // v = theta / PI
    
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
    color = equirectangularMap.Sample(LinearWrapSampler, uv).rgb;

    return float4(color, 1.0);
}