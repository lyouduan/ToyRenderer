#include "Common.hlsl"
#include "Utlis.hlsl"

#ifndef PI 
#define PI 3.1415926
#endif

static const float NUM_STEPS = 6;
static const float NUM_DIRECTIONS = 6;
static const float u_AOStrength = 5;

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

float2 SnapUVOffset(float2 uv)
{
    return round(uv * ScreenDimensions) * InvScreenDimensions;
}

float TanToSin(float x)
{
    return x * (1.0f / (float)(x * x + 1.0));
}

float InvLength(float2 V)
{
    return 1.0f / (float)(dot(V, V));
}

float Tangent(float3 V)
{
    return V.z * InvLength(V.xy);
}

float Tangent(float3 P, float3 S)
{
    return -(P.z - S.z) * InvLength(S.xy - P.xy);
}

float BiasedTangent(float3 V)
{
    float TanBias = tan(30.0 * PI / 180.0);
    return V.z * InvLength(V.xy) + TanBias;
}


float Falloff(float d2)
{
    float NegInvR2 = -1.0 / (0.3 * 0.3);
    return d2 * NegInvR2 + 1.0f;
}

float HorizonOcclusion(float2 deltaUV, float3 P, float3 dPdu, float3 dPdv, float randStep, float numSamples, float2 uv)
{
    float ao = 0;
    uv += SnapUVOffset(randStep * deltaUV);
    
    float3 T = deltaUV.x * dPdu + deltaUV.y * dPdv;
    float tanH = BiasedTangent(T);
    float sinH = TanToSin(tanH);
    
    float tanS;
    float d2;
    float3 S;
    float R2 = 0.3 * 0.3;
    [loop]
    for (float s = 1; s <= numSamples; ++s)
    {
        uv += deltaUV;
        S = GetViewPos(uv);
        tanS = Tangent(P, S);
        d2 = Length2(S - P);
        if (d2 < R2 && tanS > tanH)
        {
            float sinS = TanToSin(tanS);
            ao += Falloff(d2) * (sinS - sinH);
            tanH = tanS;
            sinH = sinS;
        }
    }
    
    return ao;

}

float ComputeAO(float3 P, float3 N, float3 S)
{
    float3 V = S - P;
    float VdotV = dot(V, V);
    float NdotV = dot(N, V) * 1.0 / sqrt(VdotV);

  // Use saturate(x) instead of max(x,0.f) because that is faster on Kepler
    return clamp(NdotV - 0.01, 0, 1) * clamp(Falloff(VdotV), 0, 1);
}

float3 ReconstructNormal(float2 UV, float3 P)
{
    float3 Pr = GetViewPos(UV + float2(InvScreenDimensions.x, 0));
    float3 Pl = GetViewPos(UV + float2(-InvScreenDimensions.x, 0));
    float3 Pt = GetViewPos(UV + float2(0, InvScreenDimensions.y));
    float3 Pb = GetViewPos(UV + float2(0, -InvScreenDimensions.y));
    return normalize(cross(MinDiff(P, Pr, Pl), MinDiff(P, Pt, Pb)));
}

float PS(PSInput pin) : SV_Target
{
    // Get viewspace normal
    float3 normal = NormalTexture.Sample(PointClampSampler, pin.tex).rgb;
    float3 normalV = mul(normal, (float3x3) gViewMat);
    normalV = normalize(normalV);
    
    // reconstruct P in view space 
    float3 P = GetViewPos(pin.tex);
    //float3 normalV = ReconstructNormal(pin.tex, P);
    
    // Sample nerighboring pixels
    float3 Pr = GetViewPos(pin.tex + float2( 1, 0) * InvScreenDimensions);
    float3 Pl = GetViewPos(pin.tex + float2(-1, 0) * InvScreenDimensions);
    float3 Pt = GetViewPos(pin.tex + float2( 0, 1) * InvScreenDimensions);
    float3 Pb = GetViewPos(pin.tex + float2( 0,-1) * InvScreenDimensions);
    
    // calculate the tangent basis vectors using the minimu difference
    float3 dPdu = MinDiff(P, Pr, Pl);
    float3 dPdv = MinDiff(P, Pt, Pb) * (ScreenDimensions.y * InvScreenDimensions.x);
    
    // Get the random samples from the noise
    //float3 random = GenerateNoise(pin.tex);
   
    float R = 0.3;
    float2 FocalLen = float2(1, 1);
    float2 rayRadiusUV = (0.5 * R * FocalLen) / P.z;
    float rayRadiusPix = rayRadiusUV.x * ScreenDimensions.x;
    rayRadiusPix = max(rayRadiusPix, 1.0);
    
    float ao = 1.0;
   // Divide by NUM_STEPS+1 so that the farthest samples are not fully attenuated
    
    if (rayRadiusPix > 1.0)
    {
        ao = 0.0;
        float StepSizePixels = rayRadiusPix / (NUM_STEPS + 1);
        float alpha = 2.0 * PI / NUM_DIRECTIONS;
        
        for (float DirectionIndex = 0.0; DirectionIndex < NUM_DIRECTIONS; ++DirectionIndex)
        {
            int randomIdx = (DirectionIndex / NUM_DIRECTIONS) * 16;
            float3 random = RandomSB[randomIdx];
    
            
            float Angle = alpha * DirectionIndex;
            //Compute normalized 2D direction
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
        ao *= u_AOStrength / (NUM_DIRECTIONS * NUM_STEPS);
        ao =  clamp(1.0 - ao * 2.0, 0, 1);
    }
        
    return ao;
}