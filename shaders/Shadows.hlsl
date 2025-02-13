#ifndef SHADOWS_H
#define SHADOWS_H

#include "Common.hlsl"
#include "sample.hlsl"

#define PCF_SAMPLER_COUNT 30
#define PCF_SAMPLER_PIXLE_RADIUS 3

#define BLOCKER_SEARCH_SAMPLE_COUNT    10
#define BLOCKER_SEARCH_PIXEL_RADIUS    5.0f 

Texture2D ShadowMap;

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

float PCSS(float3 ShadowPos, float bias)
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
    return PCF(ShadowPos, radius, bias);
}


float VSSM(float3 ShadowPos, float bias)
{
    
    // TODO
    
    
    return 0.0f;
}
#endif