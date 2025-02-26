#include "Common.hlsl"
#include "Utlis.hlsl"

#ifndef PI 
#define PI 3.1415926
#endif

static const float NUM_STEPS = 4;
static const float NUM_DIRECTIONS = 8;
static const float RADIUS = 8;
static const float u_AOStrength = 3.9;

Texture2D NormalTexture;
Texture2D DepthTexture;

StructuredBuffer<float3> ssaoKernel;
StructuredBuffer<float3> RandomSB;

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

float Length2(float3 V)
{
    return dot(V, V);
}

float3 MinDiff(float3 P, float3 Pr, float3 Pl)
{
    float3 V1 = Pr - P;
    float3 V2 = P - Pl;
    return (Length2(V1) < Length2(V2)) ? V1 : V2;
}

float3 GetViewPos(float2 uv)
{
	//float z = ViewSpaceZFromDepth(texture(texture0, uv).r);
    float z = DepthTexture.Sample(PointClampSampler, uv).r;
    return UVToView(uv, z, gInvProjMat).xyz;
}

// 生成伪随机噪声
float3 GenerateNoise(float2 texCoord)
{
    float noiseX = frac(sin(dot(texCoord, float2(12.9898, 78.233))) * 43758.5453);
    float noiseY = frac(sin(dot(texCoord, float2(93.9898, 47.233))) * 24634.6345);
    float noiseZ = frac(sin(dot(texCoord, float2(32.1234, 11.5678))) * 16216.8796);
    return normalize(float3(noiseX, noiseY, noiseZ) * 2.0 - 1.0);
}

float2 RotateDirections(float2 Dir, float2 CosSin)
{
    return float2(Dir.x * CosSin.x - Dir.y * CosSin.y,
                  Dir.x * CosSin.y + Dir.y * CosSin.x);
}

float Falloff(float d2)
{
    float NegInvR2 = -1.0 / (0.3 * 0.3);
    return d2 * NegInvR2 + 1.0f;
}


float ComputeAO(float3 P, float3 N, float3 S)
{
    float3 V = S - P;
    float VdotV = dot(V, V);
    float NdotV = dot(N, V) * (1.0 / sqrt(VdotV));

   // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
    //return clamp(NdotV - 0.01, 0, 1) * clamp(Falloff(VdotV), 0, 1);
    return saturate(NdotV - 0.1) * saturate(Falloff(VdotV));
}

float3 ReconstructNormal(float2 UV, float3 P)
{
    float3 Pr = GetViewPos(UV + float2(InvScreenDimensions.x, 0));
    float3 Pl = GetViewPos(UV + float2(-InvScreenDimensions.x, 0));
    float3 Pt = GetViewPos(UV + float2(0, InvScreenDimensions.y));
    float3 Pb = GetViewPos(UV + float2(0, -InvScreenDimensions.y));
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

static const float2x2 UNIFORM_DIRECTIONS[4] =
{
    float2x2(cos(0), sin(0), -sin(0), cos(0)),
  float2x2(cos(PI * 0.5), sin(PI * 0.5), -sin(PI * 0.5), cos(PI * 0.5)),
  float2x2(cos(PI), sin(PI), -sin(PI), cos(PI)),
  float2x2(cos(PI * 3.0 / 2.0), sin(PI * 3.0 / 2.0), -sin(PI * 3.0 / 2.0), cos(PI * 3.0 / 2.0))
};

float PS(PSInput pin) : SV_Target
{
    // Get viewspace normal
    float3 normal = NormalTexture.Sample(PointClampSampler, pin.tex).rgb;
    float3 normalV = mul(normal, (float3x3) gViewMat);
    normalV = normalize(normalV);
    
    // reconstruct P in view space 
    float3 P = GetViewPos(pin.tex);
    
    float ao = 0.0;
        
    float alpha = 2.0 * PI / NUM_DIRECTIONS;
    float StepSizePixels = RADIUS / (NUM_STEPS + 1) * InvScreenDimensions;
    for (float DirectionIndex = 0.0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
    {
        int randomIdx = (DirectionIndex / NUM_DIRECTIONS) * 16;
        float3 random = RandomSB[randomIdx];
    
        //Compute normalized 2D direction
        float Angle = alpha * DirectionIndex;
        float2 Direction = RotateDirections(float2(cos(Angle), sin(Angle)), random.xy);

        // Jitter starting sample within the first step
        float RayPixels = (random.z * StepSizePixels + 1.0);
        for (float StepIndex = 0; StepIndex < NUM_STEPS; ++StepIndex)
        {
            float2 SnappedUV = round(RayPixels * Direction) * InvScreenDimensions + pin.tex;
            float3 S = GetViewPos(SnappedUV);
                
            RayPixels += StepSizePixels;
            ao += ComputeAO(P, normalV, S);
        }
    }
    
    ao *= u_AOStrength * 1.0 / (NUM_DIRECTIONS * NUM_STEPS);
    ao = saturate(1.0 - ao);
    
    return ao;
}