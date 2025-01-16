
cbuffer objCBuffer : register(b0)
{
    float4x4 gModelMat;
}

cbuffer passCBuffer : register(b1)
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    
    float3 gEyePosW;
    float pad0;
    
    float3 gLightPos;
    float pad1;
    float3 gLightColor;
    float pad2;
}

cbuffer matCBuffer : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
    
    float3 gEmissvieColor;
    float gMetallic;
}

Texture2D diffuseMap;
Texture2D metallicMap;
Texture2D normalMap;
Texture2D roughnessMap;

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

struct VSOutput
{
    float4 position : SV_Position;
    float3 positionW : POSITION;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct PSOutput
{
    float4 GBufferAlbedo : SV_Target0;
    float4 GBufferSpecular : SV_Target1;
    float4 GBufferWorldPos : SV_Target2;
    float4 GBufferNormal : SV_Target3;
};


VSOutput VSMain(VSInput vin)
{
    VSOutput vout = (VSOutput) 0.0f;
    
    // transform to world space
    vout.positionW = mul(float4(vin.position.xyz, 1.0), gModelMat);
    
    // transform to clip space
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vout.positionW, 1.0), viewProj);
    
    vout.tex = vin.tex;
    
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    float3x3 NormalMatrix = (float3x3) gModelMat;
    vout.normal = mul(vin.normal, NormalMatrix);
    
    return vout;
}

PSOutput PSMain(VSOutput pin) //: SV_Target
{
    PSOutput pout;
 
    pout.GBufferWorldPos = float4(pin.positionW, 0.0f);
    pout.GBufferNormal = float4(normalize(pin.normal), 0.0f);
    float3 Albedo = diffuseMap.Sample(LinearWrapSampler, pin.tex);
    pout.GBufferAlbedo = float4(Albedo, 1.0f);
    pout.GBufferSpecular = metallicMap.Sample(LinearWrapSampler, pin.tex).r;
    
    return pout;
}