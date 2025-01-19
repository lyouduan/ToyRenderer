#include "CullLightingCommon.hlsl"

#ifndef BLOCK_SIZE
#pragma message("BLOCK_SIZE undefined. Default to 16.")
#define BLOCK_SIZE 16 // should be defined by the application.
#endif

struct ComputeShaderInput
{
    uint3 groupID : SV_GroupID; // 3D index of the thread group in the dispatch.
    uint3 groupThreadID : SV_GroupThreadID; // 3D index of local thread ID in a thread group.
    uint3 dispatchThreadID : SV_DispatchThreadID; // 3D index of local thread ID in a thread group.
    uint groupIndex : SV_GroupIndex; // Flattened local index of the thread within a thread group.
};

// Global variables
cbuffer DispatchParams
{
    // Number of groups dispatched.
    uint3 numThreadGroups;
    // uint padding //implicit padding to 16 bytes 
    
    // Total number of threads dispatched.
    // Note: This value may be less than the actual number of threads executed
    // if the screen size is not evenly divisible by the block size
    uint3 numThreads;
    // uint padding //implicit padding to 16 bytes 
};

// View space frustums for the grid cells
RWStructuredBuffer<Frustum> out_Frustums;

// A kernel to compute frustums for the grid
// This kernel is executed once per grid cell.
// Each thread computes a frustum for a grid cell.

[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CS_ComputeFrustums(ComputeShaderInput In)
{
    // View space eye position in always at the origin.
    const float3 eyePos = float3(0.0, 0.0, 0.0);
    
    // Compute the 4 corner points on the far clipping plane
    // to use as the frustum vertices.
    float4 screenSpace[4];
    
    // Top left point
    screenSpace[0] = float4(In.dispatchThreadID.xy * BLOCK_SIZE, 1.0f, 1.0f);
    // Top right point
    screenSpace[1] = float4(float2(In.dispatchThreadID.x + 1, In.dispatchThreadID.y) * BLOCK_SIZE, 1.0f, 1.0f);
    // Bottom left point
    screenSpace[2] = float4(float2(In.dispatchThreadID.x, In.dispatchThreadID.y + 1) * BLOCK_SIZE, 1.0f, 1.0f);
    // Bottom right point
    screenSpace[3] = float4(float2(In.dispatchThreadID.x + 1, In.dispatchThreadID.y + 1) * BLOCK_SIZE, 1.0f, 1.0f);
    
    float3 viewSpace[4];
    // Now convert the screen space points to view space
    for (int i = 0; i < 4; i++)
    {
        viewSpace[i] = ScreenToView(screenSpace[i]).xyz;
    }

    // Now build the frustum planes from the view space
    Frustum frustum;
    
    // screen Space points
    //          top
    //     p0 -------- p1
    //     |  -------- |
    // left|  -------- | right
    //     |  -------- |
    //     p2 -------- p3
    //          bottom
    
    // Note: frustum normal toward inside for simplifying frustum culling 
    // So we need to notice the order of the three points to ComputePlane function.
    
    // left plane
    frustum.Planes[0] = ComputePlane(eyePos, viewSpace[2], viewSpace[0]);
    // right plane
    frustum.Planes[1] = ComputePlane(eyePos, viewSpace[1], viewSpace[3]);
    // top plane
    frustum.Planes[2] = ComputePlane(eyePos, viewSpace[0], viewSpace[1]);
    // bottom plane
    frustum.Planes[3] = ComputePlane(eyePos, viewSpace[3], viewSpace[2]);
    
    // Store the computed frustum in global memory (if our thread ID is in bounds of the grid)
    if (In.dispatchThreadID.x < numThreads.x && In.dispatchThreadID.y < numThreads.y)
    {
        uint index = In.dispatchThreadID.x + (In.dispatchThreadID.y * numThreads.x);
        
        out_Frustums[index] = frustum;
    }
}