#include "Common.hlsl"
#include "Utlis.hlsl"

#ifndef PI 
#define PI 3.1415926
#endif

Texture2D NormalTexture;
Texture2D DepthTexture;
Texture2D WorldPosTexture;

StructuredBuffer<float3> ssaoKernel;

cbuffer cbSSAO
{
    float4x4 gProjTex;

    // Coordinates given in view space.
    float gOcclusionRadius;
    float gOcclusionFadeStart;
    float gOcclusionFadeEnd;
    float gSurfaceEpsilon;
};


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

static const int gSampleCount =64;

// Determines how much the point R occludes the point P as a function of DistZ.
float OcclusionFunction(float DistZ)
{
	//
	//       1.0     -------------\
	//               |           |  \
	//               |           |    \
	//               |           |      \ 
	//               |           |        \
	//               |           |          \
	//               |           |            \
	//  ------|------|-----------|-------------|---------|--> zv
	//        0     Eps       Start           End       
	//
	
    float Occlusion = 0.0f;
    if (DistZ > gSurfaceEpsilon)
    {
        float FadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;
        Occlusion = saturate((gOcclusionFadeEnd - DistZ) / FadeLength);
    }
	
    return Occlusion;
}

// 生成伪随机噪声
float3 GenerateNoise(float2 texCoord)
{
    float noiseX = frac(sin(dot(texCoord, float2(12.9898, 78.233))) * 43758.5453);
    float noiseY = frac(sin(dot(texCoord, float2(93.9898, 47.233))) * 24634.6345);
    float noiseZ = frac(sin(dot(texCoord, float2(32.1234, 11.5678))) * 16216.8796);
    return normalize(float3(noiseX, noiseY, noiseZ) * 2.0 - 1.0);
}

float PS(PSInput pin) : SV_Target
{
    // Get viewspace normal
    float3 normal = NormalTexture.Sample(PointClampSampler, pin.tex).rgb;
    float3 normalV = mul(normal, (float3x3)gViewMat);
    normalV = normalize(normalV);
    
    // reconstruct P in view space 
    float depth = DepthTexture.Sample(PointClampSampler, pin.tex).r;
    float3 P = UVToView(pin.tex, depth, gInvProjMat).xyz;
    
    //float3 randomVec = GenerateNoise(pin.tex);
    //float3 tangent = normalize(randomVec - normalV * dot(randomVec, normalV));
    //float3 bitangent = cross(normalV, tangent);
    //float3x3 TBN = float3x3(tangent, bitangent, normalV);
    
    float Occlusion = 0.0f;
    
    // Sample neighboring points about P in the hemisphere oriented by normal.
    const float Phi = PI * (3.0f - sqrt(5.0f));
    for (int i = 0; i < gSampleCount; ++i)
    {
        // Fibonacci lattices
        float3 Offset;
        Offset.y = 1 - (i / float(gSampleCount - 1)) * 2; // y goes from 1 to -1
        float Radius = sqrt(1.0f - Offset.y * Offset.y); // Radius at y
        float Theta = Phi * i; // Golden angle increment
        Offset.x = Radius * cos(Theta);
        Offset.z = Radius * sin(Theta);
        
        //float3 sampleDir = normalize(mul(ssaoKernel[i], TBN));
        //float3 Q = P + sampleDir * gOcclusionRadius;
        // Flip offset if it is behind P
        float Flip = sign(dot(Offset, normalV));
        // Sample point Q
        float3 Q = P + Flip * gOcclusionRadius * Offset;
        
        // Get potential occluder R
        float2 UV = ViewToUV(float4(Q, 1.0f), gProjMat);
        float Rz = DepthTexture.Sample(PointClampSampler, UV).r;
        float3 R = UVToView(UV, Rz, gInvProjMat).xyz;
        
        // Test whether R occludes P
        float AngleFactor = max(dot(normalV, normalize(R - P)), 0.0f);
        float DistFactor = OcclusionFunction(P.z - R.z);
        
        Occlusion += AngleFactor * DistFactor;
    }
    
    Occlusion /= gSampleCount;
	
    float AmbientAccess = 1.0f - Occlusion;

	// Sharpen the contrast of the SSAO map to make the SSAO affect more dramatic.
    return saturate(pow(AmbientAccess, 6.0f));
}