#ifndef SHADOWS_H
#define SHADOWS_H

#include "Common.hlsl"
#include "sample.hlsl"

#define PCF_SAMPLER_COUNT 30
#define PCF_SAMPLER_PIXLE_RADIUS 5

#define BLOCKER_SEARCH_SAMPLE_COUNT    10
#define BLOCKER_SEARCH_PIXEL_RADIUS    5.0f 

#define VSSM_MAX_MIP_LEVEL 5

#define CSM_MAX_COUNT 4

Texture2D ShadowMap;
Texture2D VSMTexture;
Texture2D ESMTexture;
Texture2D EVSMTexture;
Texture2D VSSMTextures[VSSM_MAX_MIP_LEVEL];

cbuffer CSMCBuffer
{
    float frustumVSFarZ[CSM_MAX_COUNT];
};

Texture2D CSMTextures[CSM_MAX_COUNT];


static const float2 offsets[9] =
{
    float2(-1,-1), float2(0,-1), float2(1,-1),
    float2(-1, 0), float2(0, 0), float2(1, 0),
    float2(-1, 1), float2(0, 1), float2(1,-1),
};

float PCF(float3 ShadowPos, float radius, float bias)
{
    float curDepth = ShadowPos.z;
    
    uint width, height, numMips;
    ShadowMap.GetDimensions(0, width, height, numMips);
    float2 tileSize = 1.0f / float2(width, height);
    
    float percentLit = 0.0f;
    for (int i = 0; i < PCF_SAMPLER_COUNT; ++i)
    {
        //float SampleUV = ShadowPos.xy + offsets[i] * tileSize;
        float2 SampleUV = ShadowPos.xy + (Hammersley(i, PCF_SAMPLER_COUNT) * 2.0f - 1.0f) * radius * tileSize;
        
        float depth = ShadowMap.Sample(LinearClampSampler, SampleUV).r;
        if(depth + bias > curDepth)
            percentLit += 1;
    }
  
    return percentLit / PCF_SAMPLER_COUNT;
}


float BlockerSearch(float3 ShadowPos, float dx)
{
    float AverageBlockerDepth = 0.0f;
    int BlockerCount = 0;
    
    const float SearchWidth = BLOCKER_SEARCH_SAMPLE_COUNT * dx;
    
    float ReceiverDepth = ShadowPos.z;
    
    for (int i = 0; i < BLOCKER_SEARCH_SAMPLE_COUNT; ++i)
    {
        float2 SampleUV = ShadowPos.xy + (Hammersley(i, BLOCKER_SEARCH_SAMPLE_COUNT) * 2.0f - 1.0f) * SearchWidth;
        
        float BlockerDepth = ShadowMap.Sample(LinearClampSampler, SampleUV).r;
        
        if (BlockerDepth < ReceiverDepth)
        {
            AverageBlockerDepth += BlockerDepth;
            BlockerCount++;
        }
    }
    
    if (BlockerCount > 0)
    {
        AverageBlockerDepth /= BlockerCount;
    }
    else
    {
        AverageBlockerDepth = -1.0f;
    }
      
    return AverageBlockerDepth;
}

float PCSS(float3 ShadowPos)
{
    float ReceiverDepth = ShadowPos.z;
    
    // 1. calcualte average Blocker depth
    uint width, height, numMips;
    ShadowMap.GetDimensions(0, width, height, numMips);
    float2 tileSize = 1.0f / float2(width, height);
    
    float blockerDepth = BlockerSearch(ShadowPos.xyz, tileSize.x);
    
    if (blockerDepth < 0.0f)
        return 1.0f;
    
    // 2. Calculate the Penumbra radius
    float lightSize = 20.0;
    float radius = (lightSize * (ReceiverDepth - blockerDepth)) / blockerDepth;
    
    // 3. PFC using Radius
    float bias = 0.001f;
    return PCF(ShadowPos, radius, bias);
}

float Chebyshev(float2 moment, float t)
{
    // One-tailed inequality valid if t > moment.x
    float p = (t <= moment.x);
    
    // (sigma^2 = E[x^2] - E[x]^2)
    float Variance = moment.y - moment.x * moment.x;
    float Diff = t - moment.x;
    // p = sigma^2 / (sigma^2 + (d - E[x])^2)
    float p_max = Variance / (Variance + Diff * Diff);
    
    return max(p, p_max);
}

float VSM(float3 ShadowPos)
{
    float ReceiverDepth = ShadowPos.z;
    float2 SampleValue = VSMTexture.Sample(LinearClampSampler, ShadowPos.xy).xy;
  
    float Result = Chebyshev(SampleValue, ReceiverDepth);
    
    return saturate(Result);
}

float ESM(float3 ShadowPos)
{
    float ReceiverDepth = ShadowPos.z;
    float SampleValue = ESMTexture.Sample(LinearClampSampler, ShadowPos.xy).x;
    
    float Result = exp(-60 * ReceiverDepth) * SampleValue.x;
    
    return saturate(Result);
}

float EVSM(float3 ShadowPos)
{
    float ReceiverDepth = ShadowPos.z;
    float4 SampleValue = EVSMTexture.Sample(LinearClampSampler, ShadowPos.xy).xyzw;
  
    float warpDepth1 = exp(30 * ReceiverDepth);
    float warpDepth2 = -exp(-30 * ReceiverDepth);
    
    float p1 = Chebyshev(SampleValue.xy, warpDepth1);
    float p2 = Chebyshev(SampleValue.zw, warpDepth2);
    
    return min(p1, p2);
}

float2 GetVSSM(float Radius, float3 ShadowPos)
{
    float Level = Radius * (VSSM_MAX_MIP_LEVEL - 1);
    int FloorLevel = floor(Level);
    int CeilLevel = ceil(Level);

    float2 FloorSample = VSSMTextures[FloorLevel].Sample(LinearWrapSampler, ShadowPos.xy).rg;
    float2 CeilSample = VSSMTextures[CeilLevel].Sample(LinearWrapSampler, ShadowPos.xy).rg;
	
    float2 Result = lerp(FloorSample, CeilSample, (Level - FloorLevel));
    return Result;
}

float VSSM(float3 ShadowPos)
{
    float ReceiverDepth = ShadowPos.z;
    
    // 1. calcualte average Blocker depth
    float2 SampleValue = VSSMTextures[2].Sample(LinearClampSampler, ShadowPos.xy).xy;
    if (ReceiverDepth - 0.005 <= SampleValue.x) // Receiver closer
    {
        return 1.0f;
    }
    
    float p = Chebyshev(SampleValue, ReceiverDepth);
    float Zunocc = ReceiverDepth;
    float Zavg = SampleValue.x;
    float Zocc = (Zavg - p * Zunocc) / (1 - p);
    
    // 2. Calculate the Penumbra radius
    float lightSize = 20.0;
    float radius = (lightSize * (ReceiverDepth - Zocc)) / Zocc;
    float maxRadius = 5.0;
    radius = saturate(radius / maxRadius);
    
    // VSM
    float2 VSSM_SampleValue = GetVSSM(radius, ShadowPos);
  
    float Result = Chebyshev(VSSM_SampleValue, ReceiverDepth);
    
    return Result;
}


float CSM(float4 ShadowPosH, int cascadeIdx)
{
     // Complete projection by doing division by w.
    ShadowPosH.xyz /= ShadowPosH.w;

    // NDC space.
    float3 ReceiverPos = ShadowPosH.xyz;
    if (ReceiverPos.x < 0.0f || ReceiverPos.x > 1.0f || ReceiverPos.y < 0.0f || ReceiverPos.y > 1.0f)
    {
        return 0.0f;
    }
    
    float curDepth = ShadowPosH.z;
    
    uint width, height, numMips;
    CSMTextures[cascadeIdx].GetDimensions(0, width, height, numMips);
    float2 tileSize = 1.0f / float2(width, height);
    
    float percentLit = 0.0f;
    
    float depth = CSMTextures[cascadeIdx].Sample(LinearClampSampler, ShadowPosH.xy).r;
    
    if (depth + 0.001 > curDepth)
        percentLit = 1.0;
    
    return percentLit;
}

float CalcVisibility(float4 ShadowPosH)
{
    // Complete projection by doing division by w.
    ShadowPosH.xyz /= ShadowPosH.w;

    // NDC space.
    float3 ReceiverPos = ShadowPosH.xyz;
    if (ReceiverPos.x < 0.0f || ReceiverPos.x > 1.0f || ReceiverPos.y < 0.0f || ReceiverPos.y > 1.0f)
    {
        return 0.0f;
    }

    float ShadowFactor = 1.0;
#if USE_PCSS 
    ShadowFactor = PCSS(ReceiverPos);
#elif USE_VSM
    ShadowFactor = VSM(ReceiverPos);
#elif USE_VSSM
    ShadowFactor = VSSM(ReceiverPos);
#elif USE_ESM
    ShadowFactor = ESM(ReceiverPos);
#elif USE_EVSM
    ShadowFactor = EVSM(ReceiverPos);

#else
    float bias = 0.005;
    ShadowFactor = PCF(ReceiverPos, PCF_SAMPLER_PIXLE_RADIUS, bias);
#endif 
    return ShadowFactor;
}

#endif