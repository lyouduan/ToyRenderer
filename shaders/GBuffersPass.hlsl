#include "Common.hlsl"
#include "Utlis.hlsl"

Texture2D diffuseMap;
Texture2D metallicMap;
Texture2D normalMap;
Texture2D roughnessMap;

struct VSInput
{
    float4 position : POSITION;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float2 tex : TEXCOORD;
};

struct VSOutput
{
    float4 PosH : SV_Position;
    float4 CurPosH : Position0;
    float4 PrePosH : Position1;
    float3 PosW : POSITION2;
    float3 normal : NORMAL;
    float2 tex : TEXCOORD;
};

struct PSOutput
{
    float4 GBufferAlbedo   : SV_Target0;
    float4 GBufferSpecular : SV_Target1;
    float4 GBufferWorldPos : SV_Target2;
    float4 GBufferNormal   : SV_Target3;
    float2 GBufferVelocity : SV_Target4;
};


VSOutput VSMain(VSInput vin)
{
    VSOutput vout = (VSOutput) 0.0f;
    
    // transform to world space
    float4 posW = mul(float4(vin.position.xyz, 1.0), gModelMat);
    vout.PosW = posW.xyz;
    
    // transform to clip space
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.PosH = mul(posW, viewProj);
    
    vout.tex = vin.tex;
    
    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    float3x3 NormalMatrix = (float3x3) gInvTranModelMat;
    vout.normal = mul(vin.normal, NormalMatrix);
    
    vout.CurPosH = mul(posW, viewProj);
    
    float4 PrePosW = mul(float4(vin.position.xyz, 1.0f), gModelMat);
    vout.PrePosH = mul(PrePosW, gPreViewProjMat);
    
    return vout;
}

PSOutput PSMain(VSOutput pin) //: SV_Target
{
    PSOutput pout;
 
    pout.GBufferWorldPos = float4(pin.PosW, 0.0f);
    pout.GBufferNormal = float4(normalize(pin.normal), 0.0f);
    float3 Albedo = diffuseMap.Sample(LinearWrapSampler, pin.tex);
    pout.GBufferAlbedo = float4(Albedo, 1.0f);
    pout.GBufferSpecular = metallicMap.Sample(LinearWrapSampler, pin.tex).r;
    
    // Velocity
    float4 CurPos = pin.CurPosH;
    CurPos /= CurPos.w; // Complete projection division
    CurPos.xy  = NDCToUV(CurPos);
    
    float4 PrePos = pin.PrePosH;
    PrePos /= PrePos.w; // Complete projection division
    PrePos.xy = NDCToUV(PrePos);
    
    pout.GBufferVelocity = CurPos.xy - PrePos.xy;
    
    return pout;
}