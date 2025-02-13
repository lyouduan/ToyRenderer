#include "Common.hlsl"
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

TextureCube EnvironmentMap : register(t0);

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
    float3 irradiance = float3(0, 0, 0);
    
    float3 N = normalize(pin.positionW);
    
    //float3 up = float3(0.0, 1.0, 0.0);
    //float3 right = normalize(cross(up, N));
    //up = normalize(cross(N, right));
    //
    //float sampleDelta = 0.25;
    //float nrSamples = 0.0;
    //for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    //{
    //    for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
    //    {
    //        float3 tangentSample = float3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
    //        float3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * N;
    //        irradiance += EnvironmentMap.Sample(LinearWrapSampler, sampleVec).rgb * cos(theta) * sin(theta);
    //        nrSamples++;
    //    }
    //}
    //irradiance = PI * irradiance * (1 / (float) nrSamples);
   
    const uint SAMPLE_NUM = 1024u;
    for (uint i = 0u; i < SAMPLE_NUM; i++)
    {
       float2 Xi = Hammersley(i, SAMPLE_NUM);
       float3 L = CosOnHalfSphere(Xi, N);
       irradiance += EnvironmentMap.Sample(LinearWrapSampler, L).rgb;
    }
    irradiance *= 1.0 / float(SAMPLE_NUM);
    
    return float4(irradiance, 1.0);
}