#include "CullLightingCommon.hlsl"
#include "lightInfo.hlsl"

#define MAX_LIGHT_COUNT_IN_TILE 100

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

// 
// ------------------ Light Culling ------------------------
// 

// the depth from the screen space texture
Texture2D DepthTexture;
// Pre-computeed frustum for the grid.
StructuredBuffer<Frustum> in_Frustums;

// Golbal counter for current index into the light list
globallycoherent RWStructuredBuffer<uint> o_LightIndexCounter;

// Light index lists and light grids.
RWStructuredBuffer<uint> o_LightIndexList;
RWTexture2D<float4> DebugTexture;

groupshared uint uMinDepth;
groupshared uint uMaxDepth;

// groupshared
groupshared uint MinTileDepthInt;
groupshared uint MaxTileDepthInt;

groupshared uint TileLightIndices[MAX_LIGHT_COUNT_IN_TILE];
groupshared uint TileLightCount;

struct TileLightInfo
{
    uint LightIndices[MAX_LIGHT_COUNT_IN_TILE];
    uint LightCount;
};
RWStructuredBuffer<TileLightInfo> TileLightInfoList;

float4 NDCToView(float4 NDCPos, float4x4 InvProj)
{
    float4 View = mul(NDCPos, InvProj);
    View /= View.w;
 
    return View;
}

// Light Culling Compute Shader
[numthreads(BLOCK_SIZE, BLOCK_SIZE, 1)]
void CS_main(ComputeShaderInput In)
{
    uint TileCountX = ceil(ScreenDimensions.x / BLOCK_SIZE);
    uint TileIndex = In.groupID.y * TileCountX + In.groupID.x;
    
    if (In.groupIndex == 0) // Avoid contention by other threads in the group.
    {
        uMinDepth = 0xFFFFFFFF;
        uMaxDepth = 0;
        TileLightCount = 0;
    }
    // Wait for all threads to finish.
    //GroupMemoryBarrierWithGroupSync();
    
    // Calculate min & max depth in threadgroup / tile.
    int2 texCoord = In.dispatchThreadID.xy;
    
     // Convert depth values to view space.
    //CalculateMinMaxDepthInLds(texCoord);
    
    float Depth = DepthTexture[texCoord].r;
    uint DepthInt = asuint(Depth);
	
    InterlockedMin(uMinDepth, DepthInt);
    InterlockedMax(uMaxDepth, DepthInt);
	
    GroupMemoryBarrierWithGroupSync();
    float MinTileDepth = asfloat(uMinDepth);
    float MaxTileDepth = asfloat(uMaxDepth);
 
 
    // Clipping plane for minimum depth value 
    // (used for testing lights within the bounds of opaque geometry).
    //Plane minPlane = { float3(0, 0, 1), fMinDepth };
    //float fMinDepthVS = ClipToView(float4(0.0, 0.0, fMinDepth, 1.0));
    //float fMaxDepthVS = ClipToView(float4(0.0, 0.0, fMaxDepth, 1.0));
   
    float NearZ = NDCToView(float4(0, 0, MinTileDepth, 1.0f), gInvProjMat).z;
    float FarZ = NDCToView(float4(0, 0, MaxTileDepth, 1.0f), gInvProjMat).z;
    
    // Cull lights
    // Each thread in a group will cull 1 light until all light have been culled.
    
    float3 lightColor = float3(1.0, 1.0, 1.0);
    for (int lightIndex = In.groupIndex; lightIndex < NUM_LIGHTS; lightIndex += BLOCK_SIZE * BLOCK_SIZE)
    {
        Light light = Lights[lightIndex];
       
        // Point light
        float3 lightPosVS = mul(float4(light.PositionW.xyz, 1), gViewMat).xyz;
        Sphere sphere = { lightPosVS, light.Range };
        
        //Sphere sphere = { light.PositionV.xyz, light.Range };
        
        lightColor = float3(1.0, 1.0, 1.0);
        
        if (SphereInsideFrustum(sphere, In.groupID, NearZ, FarZ))
        {
            //lightColor = light.PositionW.xyz;
            // Add light to light list for transparent geometry.
            //o_AppendLight(i);
            uint index = 0; // Index into the visible lights array.
            InterlockedAdd(TileLightCount, 1, index); // make sure atomic add updated by a single thread a time in multi-threads
            TileLightIndices[index] = lightIndex;
            
            //if (!SphereInsidePlane(sphere, minPlane))
            //{
            //    // Add light to light list for opaque geometry
            //    
            //    lightColor = float3(1.0, 0.0, 1.0);
            //}
        }
    }
    
    // Wait till all threads in group have caught up.
    GroupMemoryBarrierWithGroupSync();
    
    // Update global memory with visible light buffer.
    // First update the light grid (only thread 0 in group needs to do this)
    if(In.groupIndex == 0)
    {
        // Update light grid for opaque geometry.
        //InterlockedAdd(o_LightIndexCounter[0], o_LightCount, o_LightIndexStartOffset);
        //o_LightGrid[In.groupID.xy] = uint2(o_LightIndexStartOffset, o_LightCount);
        // Update light grid for transparent geometry
        // TODO...
        TileLightInfoList[TileIndex].LightCount = TileLightCount;
        TileLightInfoList[TileIndex].LightIndices = TileLightIndices;

    }
    
    GroupMemoryBarrierWithGroupSync();
  
    // Now update the lightd index list (all threads).
    // For opaque geometry
    for (int i = In.groupIndex; i < TileLightCount; i += BLOCK_SIZE * BLOCK_SIZE)
    {
        TileLightInfoList[TileIndex].LightIndices[i] = TileLightIndices[i];
        
        //o_LightIndexList[o_LightIndexStartOffset + i] = o_LightList[i];
        //InterlockedAdd(o_LightIndexStartOffset, o_LightCount);
    }
    
    // ================= Debug ======================
    // Update the debug texture output.
    {
        float perc = TileLightCount / (float) NUM_LIGHTS;
        DebugTexture[texCoord] = float4(perc, perc, perc, 1.0f);
    }
}

