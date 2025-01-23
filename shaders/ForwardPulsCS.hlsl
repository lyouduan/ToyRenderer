#include "CullLightingCommon.hlsl"
#include "lightInfo.hlsl"

#ifndef BLOCK_SIZE
#pragma message("BLOCK_SIZE undefined. Default to 16.")
#define BLOCK_SIZE 16// should be defined by the application.
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

cbuffer passCBuffer
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    
    float3 gEyePosW;
    float pad0;
    
    float3 gLightPos;
    float pad1;
    float3 gLightColor;
    float pad2;
}

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

// 
// ------------------ Light Culling ------------------------
// 

// the depth from the screen space texture
Texture2D DepthTextureVS;
// Pre-computeed frustum for the grid.
StructuredBuffer<Frustum> in_Frustums;

// Golbal counter for current index into the light list
globallycoherent RWStructuredBuffer<uint> o_LightIndexCounter;

// Light index lists and light grids.
RWStructuredBuffer<uint> o_LightIndexList;
RWTexture2D<uint2> o_LightGrid;
RWTexture2D<float4> DebugTexture;

groupshared uint uMinDepth;
groupshared uint uMaxDepth;

groupshared Frustum GroupFrustum;

// Opaque geometry light lists
groupshared uint o_LightCount;
groupshared uint o_LightIndexStartOffset;
groupshared uint o_LightList[1024];


// Add the light to the visible light list for opaque geometry
void o_AppendLight(uint lightIndex)
{
    uint index = 0; // Index into the visible lights array.
    InterlockedAdd(o_LightCount, 1, index); // make sure atomic add updated by a single thread a time in multi-threads
    if(index < 1024)
    {
        o_LightList[index] = lightIndex;
    }
    
}

// Light Culling Compute Shader
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CS_main(ComputeShaderInput In)
{
    // Counters
    uint i, index;

    // Calculate min & max depth in threadgroup / tile.
    int2 texCoord = In.dispatchThreadID.xy;
    float fDepth = DepthTextureVS.Load(int3(texCoord, 0)).r;

    uint uDepth = asuint(fDepth);

    if (In.groupIndex == 0) // Avoid contention by other threads in the group.
    {
        uMinDepth = 0xffffffff;
        uMaxDepth = 0;
        o_LightCount = 0;
        GroupFrustum = in_Frustums[In.groupID.x + (In.groupID.y * numThreadGroups.x)];
    }
    InterlockedMin(uMinDepth, uDepth);
    InterlockedMax(uMaxDepth, uDepth);
    
    GroupMemoryBarrierWithGroupSync();

    float fMinDepth = asfloat(uMinDepth);
    float fMaxDepth = asfloat(uMaxDepth);

     // Convert depth values to view space.
    float minDepthVS = ClipToView(float4(0, 0, fMinDepth, 1)).z;
    float maxDepthVS = ClipToView(float4(0, 0, fMaxDepth, 1)).z;
    float nearClipVS = ClipToView(float4(0, 0, 0, 1)).z;
 
    // Clipping plane for minimum depth value 
    // (used for testing lights within the bounds of opaque geometry).
    Plane minPlane = { float3(0, 0, 1), minDepthVS };
    
    // Cull lights
    // Each thread in a group will cull 1 light until all light have been culled.
    
    float3 lightColor = float3(1.0, 1.0, 1.0);
    for (i = In.groupIndex;  i < NUM_LIGHTS; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        Light light = Lights[i];
       
        // Point light
        Sphere sphere = { mul(float4(light.PositionW.xyz, 1), gViewMat).xyz, light.Range };
        //Sphere sphere = { light.PositionV.xyz, light.Range };
        
        lightColor = float3(1.0, 1.0, 1.0);
        
        if (SphereInsideFrustum(sphere, GroupFrustum, nearClipVS, maxDepthVS))
        {
            //lightColor = light.PositionW.xyz;
            
            
            // Add light to light list for transparent geometry.
            //t_AppendLight(i);
            
            if (!SphereInsidePlane(sphere, minPlane))
            {
                lightColor = float3(1.0, 0.0, 1.0);
                
                // Add light to light list for opaque geometry
                o_AppendLight(i);
            }
        }
    }
    
    // Wait till all threads in group have caught up.
    GroupMemoryBarrierWithGroupSync();
    
    // Update global memory with visible light buffer.
    // First update the light grid (only thread 0 in group needs to do this)
    if(In.groupIndex == 0)
    {
        // Update light grid for opaque geometry.
        InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
        o_LightGrid[In.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);
        
        // Update light grid for transparent geometry
        // TODO...
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    // Now update the lightd index list (all threads).
    // For opaque geometry
    for (i = In.groupIndex; i < NUM_LIGHTS; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
    }
    
    // ================= Debug ======================
    // Update the debug texture output.
    if (o_LightCount > 0)
    {
        if(o_LightCount < 5)
            DebugTexture[texCoord] = float4(1.0, 0.0 ,0.0, 1.0f);
        if (o_LightCount < 10)
            DebugTexture[texCoord] = float4(0.0, 1.0, 0.0, 1.0f);
        if (o_LightCount < 15)
            DebugTexture[texCoord] = float4(0.0, 0.0, 1.0, 1.0f);
        if (o_LightCount < 20)
            DebugTexture[texCoord] = float4(1.0, 0.0, 1.0, 1.0f);
        else
            DebugTexture[texCoord] = float4(1.0, 1.0, 1.0, 1.0f);
    }
    else
    {
        DebugTexture[texCoord] = float4(0.0, 0.0, 0.0, 1); // None
    }
}

