Texture2D<float> InputTexture;
RWTexture2D<float> OutputTexture;

#define N 256

// 共享内存
groupshared float g_kSumShared[N];

[numthreads(N, 1, 1)]
void localCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    uint2 pixelID = uint2(DTid.x, DTid.y);
    g_kSumShared[GTid.y * N + GTid.x] = InputTexture[pixelID];
    GroupMemoryBarrierWithGroupSync();
      
    // the pixel =all the origin the pixel left +  all the origin the pixel up + self
    if (GTid.x == 0)
    {
        float3 total = float3(0.0f, 0.0f, 0.0f);
        for (int t = 0; t < N; t++)
        {
            total += g_kSumShared[t + GTid.y * N];
            g_kSumShared[t + GTid.y * N] = total;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    OutputTexture[pixelID] = g_kSumShared[GTid.y * N + GTid.x];
}



// 共享内存
groupshared float shared_data[N * 2];

struct ComputeShaderInput
{
    uint3 groupID : SV_GroupID; // 3D index of the thread group in the dispatch.
    uint3 groupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID : SV_DispatchThreadID; // 3D index of local thread ID in a thread group.
    uint groupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

[numthreads(N, 1, 1)]
void HorzCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    uint2 pixelID = uint2(DTid.x, DTid.y);
    if (GTid.x < Gid.x)
        shared_data[GTid.y * 2 * N + GTid.x] = InputTexture[uint2((GTid.x + 1) * N - 1, pixelID.y)].r;
    else
        shared_data[GTid.y * 2 * N + GTid.x] = 0.0f;
      
    if (GTid.x + N < Gid.x)
        shared_data[GTid.y * 2 * N + GTid.x + N] = InputTexture[uint2((GTid.x + 1 + N) * N - 1, pixelID.y)].r;
    else
        shared_data[GTid.y * 2 * N + GTid.x + N] = 0.0f;

    GroupMemoryBarrierWithGroupSync();
    
    for (uint t = N; t > 0; t = t / 2)
    {
        if (GTid.x < t)
            shared_data[GTid.y * 2 * N + GTid.x] += shared_data[GTid.y * 2 * N + GTid.x + t];
        GroupMemoryBarrierWithGroupSync();
    }
    GroupMemoryBarrierWithGroupSync();

    // we must transpose the x,y to next calc
    OutputTexture[uint2(pixelID.y, pixelID.x)] = shared_data[GTid.y * 2 * N] + InputTexture[pixelID].r;
}


[numthreads(N, 1, 1)]
void VertCS(uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex)
{
    uint2 pixelID = uint2(DTid.x, DTid.y);
    if (GTid.x < Gid.x)
        shared_data[GTid.y * 2 * N + GTid.x] = InputTexture[uint2((GTid.x + 1) * N - 1, pixelID.y)].r;
    else
        shared_data[GTid.y * 2 * N + GTid.x] = 0.0f;
      
    if (GTid.x + N < Gid.x)
        shared_data[GTid.y * 2 * N + GTid.x + N] = InputTexture[uint2((GTid.x + 1 + N) * N - 1, pixelID.y)].r;
    else
        shared_data[GTid.y * 2 * N + GTid.x + N] = 0.0f;

    GroupMemoryBarrierWithGroupSync();
    
    for (uint t = N; t > 0; t = t / 2)
    {
        if (GTid.x < t)
            shared_data[GTid.y * 2 * N + GTid.x] += shared_data[GTid.y * 2 * N + GTid.x + t];
        GroupMemoryBarrierWithGroupSync();
    }
    GroupMemoryBarrierWithGroupSync();

    // we must transpose the x,y to next calc
    OutputTexture[uint2(pixelID.y, pixelID.x)] = shared_data[GTid.y * 2 * N] + InputTexture[pixelID].r;
}
