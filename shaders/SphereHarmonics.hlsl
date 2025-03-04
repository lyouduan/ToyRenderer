#include "Common.hlsl"
#include "sample.hlsl"

TextureCube IrradianceMap;
RWStructuredBuffer<float3> SH_Coefficients;

cbuffer SampleSizeBuffer
{
    uint2 SAMPLE_NUM;
};

float GetSHBasis(int i, float3 N)
{
    switch (i)
    {
        case 0:
            return 0.28209479f;
        case 1:
            return 0.48860251f * N.y;
        case 2:
            return 0.48860251f * N.z;
        case 3:
            return 0.48860251f * N.x;
        case 4:
            return 1.09254843f * N.x * N.y;
        case 5:
            return 1.09254843f * N.y * N.z;
        case 6:
            return 0.31539157f * (-N.x * N.x - N.y * N.y + 2 * N.z * N.z);
        case 7:
            return 1.09254843f * (N.z * N.x);
        case 8:
            return 0.54627421f * (N.x * N.x - N.y * N.y);
        default:
            return 0.0f;
    }
}


#define THREAD_NUM 16
#define SH_DEGREE 9

groupshared float3 SH_CoeffSum[THREAD_NUM * THREAD_NUM * SH_DEGREE];

void CalSH(float3 dir, uint groupIndex)
{
    float3 irradiance = IrradianceMap.SampleLevel(LinearClampSampler, dir, 0.0).rgb;
    uint N = SAMPLE_NUM.x * SAMPLE_NUM.y;
    float A = 4 * PI / N;
    uint groupOffset = groupIndex * SH_DEGREE;
    SH_CoeffSum[groupOffset + 0] = irradiance * GetSHBasis(0, dir) * A;
    SH_CoeffSum[groupOffset + 1] = irradiance * GetSHBasis(1, dir) * A;
    SH_CoeffSum[groupOffset + 2] = irradiance * GetSHBasis(2, dir) * A;
    SH_CoeffSum[groupOffset + 3] = irradiance * GetSHBasis(3, dir) * A;
    SH_CoeffSum[groupOffset + 4] = irradiance * GetSHBasis(4, dir) * A;
    SH_CoeffSum[groupOffset + 5] = irradiance * GetSHBasis(5, dir) * A;
    SH_CoeffSum[groupOffset + 6] = irradiance * GetSHBasis(6, dir) * A;
    SH_CoeffSum[groupOffset + 7] = irradiance * GetSHBasis(7, dir) * A;
    SH_CoeffSum[groupOffset + 8] = irradiance * GetSHBasis(8, dir) * A;
}

[numthreads(THREAD_NUM, THREAD_NUM, 1)]
void CS(uint3 dispatchThreadID : SV_DispatchThreadID, uint3 groupID : SV_GroupID, uint groupIndex : SV_GroupIndex)
{
    int id = dispatchThreadID.x + dispatchThreadID.y * THREAD_NUM;
    float3 SampleDir = UniformOnSphere(Hammersley(id, THREAD_NUM * THREAD_NUM));
    
    CalSH(SampleDir, groupIndex);
    GroupMemoryBarrierWithGroupSync();
    
    uint threadCount = THREAD_NUM * THREAD_NUM;
    for (uint k = (threadCount >> 1); k > 0; k >>= 1)
    {
        if (groupIndex < k)
        {
            uint shIndex = groupIndex * SH_DEGREE;
            uint shIndex2 = (groupIndex + k) * SH_DEGREE;
            for (uint offset = 0; offset < SH_DEGREE; offset++)
            {
                SH_CoeffSum[shIndex + offset] += SH_CoeffSum[shIndex2 + offset];
            }
        }
        GroupMemoryBarrierWithGroupSync();
    }

    if (groupIndex == 0)
    {
        uint groupCountX = SAMPLE_NUM.x / THREAD_NUM;
        uint index = (groupID.y * groupCountX + groupID.x) * SH_DEGREE;
        for (uint i = 0; i < SH_DEGREE; i++)
        {
            float3 c = SH_CoeffSum[i];
            SH_Coefficients[index + i] = c;
        }
    }
  
}