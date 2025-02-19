#include "CullLightingCommon.hlsl"

#ifndef BLOCK_SIZE
#pragma message("BLOCK_SIZE undefined. Default to 16.")
#define BLOCK_SIZE 16// should be defined by the application.
#endif

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

PSInput VSMain(VSInput input)
{
    PSInput vout;
    vout.position = input.position;
    vout.tex = input.tex;
    
    return vout;
}

Texture2D<uint2> LightGrid;
StructuredBuffer<Frustum> in_Frustums;

uint GridIndex(float2 position)
{
    const uint2 pos = uint2(position);
    
    const uint tileX = ceil(1280 / BLOCK_SIZE);
    
    return (pos.x / BLOCK_SIZE) + tileX * (pos.y / BLOCK_SIZE);
}

float4 PSMain(PSInput pin) : SV_Target
{
#if 0
    const uint gridIndex = GridIndex(pin.position.xy);
    const Frustum f = in_Frustums[gridIndex];
    const uint halfTile = BLOCK_SIZE / 2;
    
    float3 color = abs(f.Planes[1].N);
    
    if (GridIndex(float2(pin.position.x + halfTile, pin.position.y)) == gridIndex &&
        GridIndex(float2(pin.position.x, pin.position.y + halfTile)) == gridIndex)
    {
        color = abs(f.Planes[0].N);
    }
    else if (GridIndex(float2(pin.position.x + halfTile, pin.position.y)) != gridIndex &&
        GridIndex(float2(pin.position.x, pin.position.y + halfTile)) == gridIndex)
    {
        color = abs(f.Planes[2].N);
    }
    else if (GridIndex(float2(pin.position.x + halfTile, pin.position.y)) == gridIndex &&
        GridIndex(float2(pin.position.x, pin.position.y + halfTile)) != gridIndex)
    {
        color = abs(f.Planes[3].N);
    }
   
    return float4(color, 1.0f);
    
#elif 0
    const uint2 pos = uint2(pin.position.xy);
    const uint tileX = ceil(1280 / BLOCK_SIZE);
    uint2 tileIndex = pos / BLOCK_SIZE;
    
    float c = (tileIndex.x + tileX * tileIndex.y) * 0.00001f;
    if (tileIndex.x % 2 == 0)
        c += 0.2f;
    if (tileIndex.y % 2 == 0)
        c += 0.2f;
    
    return float4((float3)c, 1.0f);
    
#elif 1

    // cull light grid debug
    {
        uint2 threadXY = uint2(floor(1280 / 16.0), floor(720 / 16.0));
        uint2 tileIndex = uint2(floor(pin.tex * threadXY));
    
        uint r = LightGrid[tileIndex].g;
        if (r < 1)
            return float4(0.0, 0.0, 0.0, 1.0f);
        if (r < 8)
            return float4(0.5, 0.5, 0.5, 1.0f);
        if (r < 15)
            return float4(1.0, 0.0, 0.0, 1.0f);
        if (r < 20)
            return float4(0.0, 1.0, 0.0, 1.0f);
        if (r < 40)
            return float4(0.0, 0.0, 1.0, 1.0f);
        if (r < 70)
            return float4(1.0, 0.0, 1.0, 1.0f);
        else
            return float4(1.0, 1.0, 1.0, 1.0f);
    }

#endif
}