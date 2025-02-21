#include "Common.hlsl"

#ifndef PI 
#define PI 3.1415926
#endif

Texture2D NormalTexture;
Texture2D DepthTexture;

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

float4 UVToNDC(float2 UVPos, float Depth)
{
    return float4(2 * UVPos.x - 1, 1 - 2 * UVPos.y, Depth, 1.0f);
}
float4 NDCToView(float4 NDCPos, float4x4 InvProj)
{
    float4 View = mul(NDCPos, InvProj);
    View /= View.w;
 
    return View;
}
float4 UVToView(float2 UV, float NDCDepth, float4x4 InvProj)
{
    float4 NDC = UVToNDC(UV, NDCDepth);
	
    float4 View = NDCToView(NDC, InvProj);
	
    return View;
}
float4 ViewToNDC(float4 ViewPos, float4x4 Proj)
{
    float4 NDC = mul(ViewPos, Proj);
    NDC /= NDC.w;
	
    return NDC;
}
float2 NDCToUV(float4 NDCPos)
{
    return float2(0.5 + 0.5 * NDCPos.x, 0.5 - 0.5 * NDCPos.y);
}
float2 ViewToUV(float4 ViewPos, float4x4 Proj)
{
    float4 NDC = ViewToNDC(ViewPos, Proj);
	
    float2 UV = NDCToUV(NDC);
	
    return UV;
}

static const int gSampleCount = 16;

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

float PS(PSInput pin) : SV_Target
{
    // Get viewspace normal
    float3 normal = NormalTexture.Sample(PointClampSampler, pin.tex).rgb;
    float3 normalV = mul(normal, (float3x3)gViewMat);
    normalV = normalize(normalV);
    
    // reconstruct P in view space 
    float depth = DepthTexture.Sample(PointClampSampler, pin.tex).r;
    float3 P = UVToView(pin.tex, depth, gInvProjMat).xyz;
    
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
        Offset.y = Radius * sin(Theta);
        
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