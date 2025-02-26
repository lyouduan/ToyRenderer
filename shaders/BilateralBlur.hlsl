#include "Common.hlsl"
#include "Utlis.hlsl"

#ifndef PI 
#define PI 3.1415926
#endif

Texture2D HBAOTexture;
Texture2D DepthTexture;

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


PSInput VS(VSInput vin)
{
    PSInput vout;
    vout.position = vin.position;
    vout.tex = vin.tex;
    return vout;
}

static const float KERNEL_RADIUS = 10;

float BlurFunction(float2 uv, float r, float cneter_c, float center_d, inout float w_total)
{
    float ao = HBAOTexture.Sample(PointClampSampler, uv).r;
    float depth = DepthTexture.Sample(PointClampSampler, uv).r;
    
    float sigma = float(KERNEL_RADIUS) * 0.5;
    
    float blurFalloff = 1.f / (2.f * sigma * sigma);
    
    float difference = (depth - center_d);
    float w = exp2(-r * r * blurFalloff - (difference * difference));
    
    w_total += w;
    
    return ao * w;
}

float PS(PSInput pin) : SV_Target
{
    float ao = HBAOTexture.Sample(PointClampSampler, pin.tex).r;
    float depth = DepthTexture.Sample(PointClampSampler, pin.tex).r;

    float center_c = ao;
    float center_d = depth;
    
    float c_total = center_c;
    float w_total = 1.0;
  
    for (float r = 1; r <= KERNEL_RADIUS; ++r)
    {
        float2 uv = pin.tex + InvScreenDimensions * r;
        c_total += BlurFunction(uv, r, center_c, center_d, w_total);
    }
  
    for (r = 1; r <= KERNEL_RADIUS; ++r)
    {
        float2 uv = pin.tex - InvScreenDimensions * r;
        c_total += BlurFunction(uv, r, center_c, center_d, w_total);
    }
  
    return c_total / w_total;
}