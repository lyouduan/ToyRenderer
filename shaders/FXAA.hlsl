#include "Common.hlsl"
#include "Utlis.hlsl"

#define EDGE_THRESHOLD_MIN 0.0312f
#define EDGE_THRESHOLD_MAX 0.125f
#define ITERATIONS 12
#define QUALITY(q) ((q) < 5 ? 1.0 : ((q) > 5 ? ((q) < 10 ? 2.0 : ((q) < 11 ? 4.0 : 8.0)) : 1.5))
#define SUBPIXEL_QUALITY 0.75

Texture2D ColorTexture;

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

float rgb2luma(float3 color)
{
    return sqrt(dot(color, float3(0.213, 0.715, 0.072)));
}

float4 PS(PSInput pin) : SV_Target
{
    
    float4 finalColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    float3 colorCenter = ColorTexture.Sample(LinearClampSampler, pin.tex).rgb;
    
    // Luma at the current pixel
    float lumaCenter = rgb2luma(colorCenter);
    
    // Luma at the four direct neighbors of the current pixel
    const float2 offset[8] = { 
        float2(0, -1), float2(0, 1), float2(-1, 0), float2(1, 0),
        float2(-1, -1), float2(1, 1), float2(-1, 1), float2(1, -1) };
    
    float lumaDown  = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[0] * InvScreenDimensions).rgb);
    float lumaUp    = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[1] * InvScreenDimensions).rgb);
    float lumaLeft  = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[2] * InvScreenDimensions).rgb);
    float lumaRight = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[3] * InvScreenDimensions).rgb);
    
    // find the minimum and maximum luma values
    float lumaMin = min(lumaCenter, min(min(lumaDown, lumaUp), min(lumaLeft, lumaRight)));
    float lumaMax = max(lumaCenter, max(max(lumaDown, lumaUp), max(lumaLeft, lumaRight)));
    
    // compute the delta
    float lumaRange = lumaMax - lumaMin;

    // if the luma variation is lower that a threshold (or if we are in a really drak area),
    // we are not on an edge, don't perform any AA
    if (lumaRange < max(EDGE_THRESHOLD_MIN, lumaMax * EDGE_THRESHOLD_MAX))
    {
        finalColor = float4(colorCenter, 1.0f);
        return finalColor;
    }
    
    // compute the luma values at the diagonal neighbors
    float lumaDownLeft  = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[4] * InvScreenDimensions).rgb);
    float lumaUpRight   = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[5] * InvScreenDimensions).rgb);
    float lumaUpLeft    = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[6] * InvScreenDimensions).rgb);
    float lumaDownRight = rgb2luma(ColorTexture.Sample(LinearClampSampler, pin.tex + offset[7] * InvScreenDimensions).rgb);
    
    // combine the four edges lumas(using intermediary variables for future computations with same values).
    float lumaDownUp    = lumaDown + lumaUp;
    float lumaLeftRight = lumaLeft + lumaRight;
    
    // same for corners
    float lumaLeftCorners  = lumaDownLeft  + lumaUpLeft;
    float lumaDownCorners  = lumaDownLeft  + lumaDownRight;
    float lumaRightCorners = lumaDownRight + lumaUpRight;
    float lumaUpCorners    = lumaUpRight   + lumaUpLeft;
    
    // Compute an estimation of the gradient along the horizontal and vertical axis.
    float edgeHorizontal = abs(-2.0 * lumaLeft + lumaLeftCorners) + abs(-2.0 * lumaCenter + lumaDownUp) * 2.0    + abs(-2.0 * lumaRight + lumaRightCorners);
    float edgeVertical   = abs(-2.0 * lumaUp + lumaUpCorners)     + abs(-2.0 * lumaCenter + lumaLeftRight) * 2.0 + abs(-2.0 * lumaDown + lumaDownCorners);
    
    // Is the local edge horizontal or vertical ?
    bool isHorizontal = (edgeHorizontal >= edgeVertical);
    
     // Choose the step size (one pixel) according to the edge direction.
    float stepLength = isHorizontal ? InvScreenDimensions.y : InvScreenDimensions.x;
    
    // Select the two neighboring texels lumas in the opposite direction to the local edge.
    float luma1 = isHorizontal ? lumaDown : lumaLeft;
    float luma2 = isHorizontal ? lumaUp : lumaRight;
    
    // Compute gradients in this direction.
    float gradient1 = luma1 - lumaCenter;
    float gradient2 = luma2 - lumaCenter;
    
    // Which direction is the steepest ?
    bool is1Steepest = abs(gradient1) >= abs(gradient2);
    
    // Gradient in the corresponding direction, normalized.
    float gradientScaled = 0.25 * max(abs(gradient1), abs(gradient2));
   
    // Average luma in the correct direction.
    float lumaLocalAverage = 0.0f;
    
    if(is1Steepest)
    {
        // Switch the direction
        stepLength = -stepLength;
        lumaLocalAverage = 0.5 * (luma1 + lumaCenter);
    }
    else
    {
        lumaLocalAverage = 0.5 * (luma2 + lumaCenter);
    }
    
    // Shift UV in the correct direction by half a pixel
    float2 currentUV = pin.tex;
    if(isHorizontal)
    {
        currentUV.y += stepLength * 0.5;
    }
    else
    {
        currentUV.x += stepLength * 0.5;
    }
    
    // Compute offset (for each iteration step) in the right direction.
    float2 offsetUV = isHorizontal ? float2(InvScreenDimensions.x, 0.0f) : float2(0.0f, InvScreenDimensions.y);
    // Compute UVs to explore on each side of the edge, orthogonally. The QUALITY allows us to step faster.
    float2 uv1 = currentUV - offsetUV * QUALITY(0);
    float2 uv2 = currentUV + offsetUV * QUALITY(0);
    
    // Read the lumas at both current extremities of the exploration segment, 
    // and compute the delta wrt to the local average luma.
    float lumaEnd1 = rgb2luma(ColorTexture.Sample(LinearClampSampler, uv1).rgb);
    float lumaEnd2 = rgb2luma(ColorTexture.Sample(LinearClampSampler, uv2).rgb);
    lumaEnd1 -= lumaLocalAverage;
    lumaEnd2 -= lumaLocalAverage;
    
    // if the luma delta at the current extremities are larger than the local gradient, we have readed the side of the edge.
    bool reached1 = abs(lumaEnd1) >= gradientScaled;
    bool reached2 = abs(lumaEnd2) >= gradientScaled;
    
    bool reachedBoth = reached1 && reached2;

    // if the side is not reached, we continue to explore in this direction.
    if(!reached1)
    {
        uv1 -= offsetUV * QUALITY(1);
    }
    if (!reached2)
    {
        uv2 += offsetUV * QUALITY(1);
    }
    
    // if both sides have been reached, continue to explore
    if(!reachedBoth)
    {
        for (int i = 2; i < ITERATIONS; ++i)
        {
            // If needed, read luma in 1st direction, compute delta.
            if (!reached1)
            {
                lumaEnd1 = rgb2luma(ColorTexture.Sample(LinearClampSampler, uv1).rgb);
                lumaEnd1 -= lumaLocalAverage;
            }
            
            // If needed, read luma in opposite direction, compute delta.
            if (!reached2)
            {
                lumaEnd2 = rgb2luma(ColorTexture.Sample(LinearClampSampler, uv2).rgb);
                lumaEnd2 -= lumaLocalAverage;
            }
            
            // If the luma deltas at the current extremities is larger than the local gradient, 
            // we have reached the side of the edge.
            reached1 = abs(lumaEnd1) >= gradientScaled;
            reached2 = abs(lumaEnd2) >= gradientScaled;
            reachedBoth = reached1 && reached2;
            
            // if the side is not reached, we continue to explore in this direction, with a variable quality.
            if (!reached1)
            {
                uv1 -= offsetUV * QUALITY(i);
            }
            if (!reached2)
            {
                uv2 += offsetUV * QUALITY(i);
            }
            
            // if both sides have been reached, stop the exploration.
            if (reachedBoth)
            {
                break;
            }
        }
    }
    
    // Compute the distances to each extremity of the edge.
    float distance1 = isHorizontal ? (pin.tex.x - uv1.x) : (pin.tex.y - uv1.y);
    float distance2 = isHorizontal ? (uv2.x - pin.tex.x) : (uv2.y - pin.tex.y);
    
    // in which direction is the extremity of the edge closer ?
    bool isDirection1 = distance1 < distance2;
    float distanceFinal = min(distance1, distance2);
    
    // Length of the edge.
    float edgeThickness = (distance1 + distance2);
    
    // UV offset : read in the direction of the closest side of the edge.
    float pixelOffset = -distanceFinal / edgeThickness + 0.5f;
    
    // Is the luma at center smaller than the local average ?
    bool isLumaCneterSmaller = lumaCenter < lumaLocalAverage;
    
    // if the luma at center is smaller than at its neighbors,
    // the delta luma at each and should be positive (same variation).
    // (in the direction of the closer side of the edge.)
    bool correctVariation = ((isDirection1 ? lumaEnd1 : lumaEnd2) < 0.0f) != isLumaCneterSmaller;
    
    // if the luma variation is incorrect, dot not offset.
    float finalOffset = correctVariation ? pixelOffset : 0.0f;
    
    
    // Sub-pixel shifting
    // Full weighted average of the luma over the 3x3 neighborhood.
    float lumaAverage = (1.0f / 12.0) * (2.0f * (lumaDownUp + lumaLeftRight) + lumaLeftCorners + lumaRightCorners );
    // Ratio of the delta between the global average and the center luma, over the luma range in the 3x3 neighborhood.
    float subPixelOffset1 = clamp(abs(lumaAverage - lumaCenter) / lumaRange, 0.0f, 1.0f);
    float subPixelOffset2 = (-2.0f * subPixelOffset1 + 3.0f) * subPixelOffset1 * subPixelOffset1;
    // Compute a sub-pixel offset based on this delta.
    float subPixelOffsetFinal = subPixelOffset2 * subPixelOffset2 * SUBPIXEL_QUALITY;
    
    // Pick the biggest of the two offsets.
    finalOffset = max(finalOffset, subPixelOffsetFinal);
    
    // Compute the final UV coordinates
    float2 finalUV = pin.tex;
    if(isHorizontal)
    {
        finalUV.y += finalOffset * stepLength;
    }
    else
    {
        finalUV.x += finalOffset * stepLength;
    }
    
    
    // read the color at the new UV coordinates, and use it.
    finalColor = float4(ColorTexture.Sample(LinearClampSampler, finalUV).rgb, 1.0f);

    return finalColor;
}