#include "common.hlsl"
#include "sample.hlsl"

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

TextureCube EnvironmentMap: register(t0);


PSInput VSMain(VSInput vin)
{
    PSInput vout;
    
    // Use local vertex position as cubemap lookup vector.
    vout.positionW = vin.position.xyz;
    
    //float4 posW = mul(float4(vin.position.xyz, 1.0f), ModelMat);
    
    // Remove translation from the view matrix
    float4x4 viewProj = mul(gViewMat, gProjMat);
    vout.position = mul(float4(vin.position.xyz, 1.0), viewProj);
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 N = normalize(pin.positionW);
    float3 R = N;
    float3 V = R;
    
    float3 prefilterColor = float3(0, 0, 0);
    float TotalWeight = 0.0;
    const uint SAMPLE_NUM = 1024u;
    
    for (uint i = 0u; i < SAMPLE_NUM; i++)
    {
        float2 Xi = Hammersley(i, SAMPLE_NUM);
        float3 H = ImportanceSampleGGX(Xi, N, gRoughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NoL = saturate(dot(N, L));
        if(NoL> 0.0)
        {
            prefilterColor += EnvironmentMap.Sample(LinearWrapSampler, L).rgb * NoL;
            TotalWeight += NoL;
        }
    }
    
    prefilterColor /= TotalWeight;
    
    return float4(prefilterColor, 1.0);
}