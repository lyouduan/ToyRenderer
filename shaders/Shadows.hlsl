#ifndef SHADOWS_H
#define SHADOWS_H

#include "Common.hlsl"
#include "sample.hlsl"

#define LIGHT_SIZE 10

#define PCF_SAMPLER_COUNT 16
#define PCF_SAMPLER_PIXLE_RADIUS 3

#define BLOCKER_SEARCH_SAMPLE_COUNT    32
#define BLOCKER_SEARCH_PIXEL_RADIUS    3.0f 

#define VSSM_MAX_MIP_LEVEL 5

#define CSM_MAX_COUNT 4

Texture2D ShadowMap;
Texture2D VSMTexture;
Texture2D ESMTexture;
Texture2D EVSMTexture;
Texture2D VSSMTextures[VSSM_MAX_MIP_LEVEL];

cbuffer CSMCBuffer
{
    float2 frustumVSFarZ[CSM_MAX_COUNT];
};

Texture2D CSMTextures[CSM_MAX_COUNT];
Texture2DArray CSMTextureArray;


static const float2 offsets[9] =
{
    float2(-1,-1), float2(0,-1), float2(1,-1),
    float2(-1, 0), float2(0, 0), float2(1, 0),
    float2(-1, 1), float2(0, 1), float2(1,-1),
};

float TentFilter(float2 uv)
{
    return max(0.0, (1.0 - abs(uv.x)) * (1.0 - abs(uv.y)));
}
float2 ComputeReceiverPlaneDepthBias(float3 position)
{
    float3 duvz_dx = ddx(position);
    float3 duvz_dy = ddy(position);

    float2x2 matScreentoShadow = float2x2(duvz_dx.xy, duvz_dy.xy);
    float fDeterminant = determinant(matScreentoShadow);
    
    if (abs(fDeterminant) < 1e-6)
    {
        return float2(0, 0);
    }
    float fInvDeterminant = 1.0f / fDeterminant;
    
    float2x2 matShadowToScreen = float2x2(
        matScreentoShadow._22 * fInvDeterminant, matScreentoShadow._12 * -fInvDeterminant,
        matScreentoShadow._21 * -fInvDeterminant, matScreentoShadow._11 * fInvDeterminant);
    
    // 计算 dz/du 和 dz/dv
    float2 dz_duv = mul(float2(duvz_dx.z, duvz_dy.z), matShadowToScreen);

    return dz_duv;
}


float PCF(float3 ShadowPos, float radius)
{
    // receiver plane depth bias
    float2 dz_duv = ComputeReceiverPlaneDepthBias(ShadowPos);
    
    float curDepth = ShadowPos.z;
    
    uint width, height, numMips;
    ShadowMap.GetDimensions(0, width, height, numMips);
    float2 texSize = 1.0f / float2(width, height);
    
    // calculate the rotation matrix for the poisson disk
    float2x2 R = getRandomRotationMatrix(ShadowPos.xy);
    
    float percentLit = 0.0f;
    float2 filtterRadius = radius * texSize;
    for (int i = 0; i < PCF_SAMPLER_COUNT; ++i)
    {
        // rotate the poisson disk randomly
        float2 Offset = mul(PoissonDisk(i) * filtterRadius, R);
        //float SampleUV = ShadowPos.xy + offsets[i] * texSize;
        float2 SampleUV = ShadowPos.xy + Offset;
        float depth = ShadowMap.Sample(LinearClampSampler, SampleUV).r;
        
        float z_bias = dz_duv.x * Offset.x + dz_duv.y * Offset.y;
        
        if (depth + z_bias + 0.01 > curDepth)
        {
            percentLit += 1;
        }
    }
    
    return saturate(percentLit / float(PCF_SAMPLER_COUNT));
}


float BlockerSearch(float3 ShadowPos, float dx)
{
    float AverageBlockerDepth = 0.0f;
    int BlockerCount = 0;
    
    const float SearchWidth = BLOCKER_SEARCH_SAMPLE_COUNT * dx;
    float ReceiverDepth = ShadowPos.z;
    
    // calculate the rotation matrix for the poisson disk
    float2x2 R = getRandomRotationMatrix(ShadowPos.xy);
    
    for (int i = 0; i < BLOCKER_SEARCH_SAMPLE_COUNT; ++i)
    {
        float2 Offset = mul(PoissonDisk(i) * SearchWidth, R);
        float2 SampleUV = ShadowPos.xy + Offset;
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
    
    float lightSize = float(LIGHT_SIZE);
    
    // 2. Calculate the Penumbra radius
    float radius = (lightSize * (ReceiverDepth - blockerDepth)) / blockerDepth;
    
    // 3. PFC using Radius
    return PCF(ShadowPos, radius);
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
    float lightSize = float(LIGHT_SIZE);
    float radius = (lightSize * (ReceiverDepth - Zocc)) / Zocc;
    float maxRadius = 5.0;
    radius = saturate(radius / maxRadius);
    
    // VSM
    float2 VSSM_SampleValue = GetVSSM(radius, ShadowPos);
  
    float Result = Chebyshev(VSSM_SampleValue, ReceiverDepth);
    
    return Result;
}

float PCF(float3 ShadowPos, float radius, int cascadeIdx)
{
    // receiver plane depth bias
    float2 dz_duv = ComputeReceiverPlaneDepthBias(ShadowPos);
    
    float curDepth = ShadowPos.z;
    
    uint width, height, numMips;
#if USE_CSMINST
    CSMTextureArray.GetDimensions(width, height, numMips);
#else
    CSMTextures[cascadeIdx].GetDimensions(0, width, height, numMips);
#endif
    float2 texSize = 1.0f / float2(width, height);
    
    // calculate the rotation matrix for the poisson disk
    float2x2 R = getRandomRotationMatrix(ShadowPos.xy);
    
    float percentLit = 0.0f;
    float2 filtterRadius = radius * texSize;
    for (int i = 0; i < PCF_SAMPLER_COUNT; ++i)
    {
        // rotate the poisson disk randomly
        float2 Offset = mul(PoissonDisk(i) * filtterRadius, R);
        //float SampleUV = ShadowPos.xy + offsets[i] * texSize;
        float2 SampleUV = ShadowPos.xy + Offset;
    #if USE_CSMINST
        float depth = CSMTextureArray.Sample(LinearClampSampler, float3(SampleUV, cascadeIdx)).r;
    #else
        float depth = CSMTextures[cascadeIdx].Sample(LinearClampSampler, SampleUV).r;
    #endif
        float bias = 0.01;
        float z_bias = dz_duv.x * Offset.x + dz_duv.y * Offset.y;
        
        if (depth + z_bias + bias > curDepth)
        {
            percentLit += 1;
        }
    }
    
    return saturate(percentLit / float(PCF_SAMPLER_COUNT));
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
    
    return PCF(ReceiverPos, PCF_SAMPLER_PIXLE_RADIUS, cascadeIdx);
}

float CalcVisibility(float4 ShadowPosH, float bias)
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
    ShadowFactor = PCF(ReceiverPos, PCF_SAMPLER_PIXLE_RADIUS);
#endif 
    return ShadowFactor;
}

#endif