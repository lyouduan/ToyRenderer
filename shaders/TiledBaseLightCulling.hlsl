#include "lightInfo.hlsl"
#include "common.hlsl"
#include "Utlis.hlsl"

#ifndef TILE_BLOCK_SIZE
#pragma message("TILE_BLOCK_SIZE undefined. Default to 16.")
#define TILE_BLOCK_SIZE 16// should be defined by the application.
#endif

#define MAX_LIGHT_COUNT_IN_TILE 100

struct TileLightInfo
{
    uint LightIndices[MAX_LIGHT_COUNT_IN_TILE];
    uint LightCount;
};

Texture2D DepthTexture;
RWTexture2D<float2> TiledDepthDebugTexture;

// groupshared
groupshared uint MinTileDepthInt;
groupshared uint MaxTileDepthInt;

groupshared uint TileLightIndices[MAX_LIGHT_COUNT_IN_TILE];
groupshared uint TileLightCount;

RWStructuredBuffer<TileLightInfo> TileLightInfoList;

struct TSphere
{
    float3 Center; // Center point.
    float Radius; // Radius.
};

struct TPlane
{
    float3 N; // Plane normal.
    float D; // Signed distance from origin to this plane.
};

// Compute a plane from 3 noncollinear points that form a triangle.
//     P1
//    /  \
//   /    \
//  P0-----P2
TPlane ComputePlane(float3 P0, float3 P1, float3 P2)
{
    TPlane Plane;
 
    float3 Edge1 = P1 - P0;
    float3 Edge2 = P2 - P0;
 
    Plane.N = normalize(cross(Edge1, Edge2));
    Plane.D = -dot(Plane.N, P0);
 
    return Plane;
}

struct TFrustum
{
    TPlane Planes[4]; // Left, right, top, bottom frustum planes.
	
    float NearZ;
	
    float FarZ;
};

// If the sphere is fully or partially contained within the frustum, return true.
bool SphereInsideFrustum(TSphere Sphere, TFrustum Frustum)
{
	// Check z value
    if (Sphere.Center.z - Sphere.Radius > Frustum.FarZ || Sphere.Center.z + Sphere.Radius < Frustum.NearZ)
    {
        return false;
    }
 
    // Check frustum planes
    for (int i = 0; i < 4; i++)
    {
        if (dot(Sphere.Center, Frustum.Planes[i].N) + Frustum.Planes[i].D < -Sphere.Radius) // Sphere in the negative half-space of plane
        {
            return false;
        }
    }
	
    return true;
}
bool Intersect(uint LightIndex, uint3 GroupId, float MinTileZ, float MaxTileZ)
{
	// View space eye position is always at the origin.
    const float3 EyePos = float3(0, 0, 0);
	
	// Compute the 4 corner points as the frustum vertices.
    float2 ScreenSpace[4];
    // Top left point
    ScreenSpace[0] = float2(GroupId.xy * TILE_BLOCK_SIZE);
    // Top right point
    ScreenSpace[1] = float2(float2(GroupId.x + 1, GroupId.y) * TILE_BLOCK_SIZE);
    // Bottom left point
    ScreenSpace[2] = float2(float2(GroupId.x, GroupId.y + 1) * TILE_BLOCK_SIZE);
    // Bottom right point
    ScreenSpace[3] = float2(float2(GroupId.x + 1, GroupId.y + 1) * TILE_BLOCK_SIZE);
	
	// Convert the screen space points to view space on the far clipping plane
    float3 ViewSpace[4];
    for (int i = 0; i < 4; i++)
    {
        ViewSpace[i] = ScreenToView(ScreenSpace[i].xy, ScreenDimensions, 1.0f, gInvProjMat).xyz;
    }
	
	// Build the frustum planes from the view space points
    TFrustum Frustum;
    // Left plane
    Frustum.Planes[0] = ComputePlane(EyePos, ViewSpace[0], ViewSpace[2]);
    // Right plane
    Frustum.Planes[1] = ComputePlane(EyePos, ViewSpace[3], ViewSpace[1]);
    // Top plane
    Frustum.Planes[2] = ComputePlane(EyePos, ViewSpace[1], ViewSpace[0]);
    // Bottom plane
    Frustum.Planes[3] = ComputePlane(EyePos, ViewSpace[2], ViewSpace[3]);
	// NearZ and FarZ
    Frustum.NearZ = NDCToView(float4(0, 0, MinTileZ, 1.0f), gInvProjMat).z;
    Frustum.FarZ = NDCToView(float4(0, 0, MaxTileZ, 1.0f), gInvProjMat).z;
	
    Light light = Lights[LightIndex];
	
    // PointLight
    {
        TSphere LightSphere;
        LightSphere.Center = mul(float4(light.PositionW.xyz, 1.0f), gViewMat).xyz;
        LightSphere.Radius = light.Range;
		
        return SphereInsideFrustum(LightSphere, Frustum);
    }
}


[numthreads(TILE_BLOCK_SIZE, TILE_BLOCK_SIZE, 1)]
void CS( uint3 GroupId : SV_GroupID,
            uint GroupIndex : SV_GroupIndex,
            uint3 DispatchThreadID : SV_DispatchThreadID )
{
    uint TileCountX = ceil(ScreenDimensions.x / TILE_BLOCK_SIZE);
    uint TileIndex = GroupId.y * TileCountX + GroupId.x;
    

    uint2 TexUV = DispatchThreadID.xy;
	
	//----------------------------Initialize tile data------------------------------------//
    if (GroupIndex == 0) // Only thread 0
    {
        MinTileDepthInt = 0xFFFFFFFF;
        MaxTileDepthInt = 0;
        TileLightCount = 0;
    }
	
	// Wait for all threads to finish.
    GroupMemoryBarrierWithGroupSync();
	
	//-----------------------Calculate min/max depth of tile------------------------------//	
    float Depth = DepthTexture[TexUV].r;
    uint DepthInt = asuint(Depth);
	
    InterlockedMin(MinTileDepthInt, DepthInt);
    InterlockedMax(MaxTileDepthInt, DepthInt);
	
    GroupMemoryBarrierWithGroupSync();
    float MinTileDepth = asfloat(MinTileDepthInt);
    float MaxTileDepth = asfloat(MaxTileDepthInt);
	
	// Debug
	//TiledDepthDebugTexture[TexUV] =  float2(MinTileDepth, MaxTileDepth);
	
	//-----------------------------Light culling------------------------------------------//		
    const uint ThreadCount = TILE_BLOCK_SIZE * TILE_BLOCK_SIZE;
    for (uint LightIndex = GroupIndex; LightIndex < NUM_LIGHTS; LightIndex += ThreadCount) // All threads
    {
		//if(IntersectPosition(LightIndex, GroupId))
        if (Intersect(LightIndex, GroupId, MinTileDepth, MaxTileDepth))
        {
            uint Offset;
            InterlockedAdd(TileLightCount, 1, Offset);
            TileLightIndices[Offset] = LightIndex;
        }
    }
	
    GroupMemoryBarrierWithGroupSync();
	
	//-------------------------Copy to TileLightInfoList----------------------------------//
    if (GroupIndex == 0)  // Only thread 0
    {
        TileLightInfoList[TileIndex].LightCount = TileLightCount;
    }
	
    for (uint i = GroupIndex; i < TileLightCount; i += ThreadCount) // All threads
    {
        TileLightInfoList[TileIndex].LightIndices[i] = TileLightIndices[i];
    }
	
    GroupMemoryBarrierWithGroupSync();
	
	// Debug
    float DebugColor = TileLightCount / 81.0f;
    TiledDepthDebugTexture[TexUV] = float2(DebugColor, DebugColor);
}
    
