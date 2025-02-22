#include "Common.hlsl"
#include "Utlis.hlsl"

Texture2D ColorTexture;
Texture2D PrevColorTexture;
Texture2D GBufferVelocity;
Texture2D GbufferDepth;

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


float2 getClosestOffset(float2 tex)
{
    float closestDepth = 1.0f;
    float2 closestUV = tex;

    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float2 newUV = tex + float2(i, j) * InvScreenDimensions;

            float depth = GbufferDepth.Sample(PointClampSampler, newUV).r;
            if (depth < closestDepth)
            {
                closestDepth = depth;
                closestUV = newUV;
            }
        }
    }

    return closestUV;
}

float3 RGB2YCoCgR(float3 rgbColor)
{
    float3 YCoCgRColor;

    YCoCgRColor.y = rgbColor.r - rgbColor.b;
    float temp = rgbColor.b + YCoCgRColor.y / 2;
    YCoCgRColor.z = rgbColor.g - temp;
    YCoCgRColor.x = temp + YCoCgRColor.z / 2;

    return YCoCgRColor;
}

float3 YCoCgR2RGB(float3 YCoCgRColor)
{
    float3 rgbColor;

    float temp = YCoCgRColor.x - YCoCgRColor.z / 2;
    rgbColor.g = YCoCgRColor.z + temp;
    rgbColor.b = temp - YCoCgRColor.y / 2;
    rgbColor.r = rgbColor.b + YCoCgRColor.y;

    return rgbColor;
}

// Clips towards aabb center
float3 ClipAABB(float3 AabbMin, float3 AabbMax, float3 Point)
{
    float3 Center = 0.5 * (AabbMax + AabbMin);
    float3 Extend = 0.5 * (AabbMax - AabbMin);
    
    float3 Dist = Point - Center;
    float3 DistUnit = Dist.xyz / Extend;
    DistUnit = abs(DistUnit);
    
    float MaxDistUnit = max(DistUnit.x, max(DistUnit.y, DistUnit.z));
    
    if(MaxDistUnit > 1.0)
        return Center + Dist / MaxDistUnit;
    else
        return Point; // point inside aabb
}


float4 PS(PSInput pin) : SV_Target
{
	//Compute UVoffset to last frame
    float2 UVoffset;
    
    float Depth = GbufferDepth.Sample(PointClampSampler, pin.tex).r;
    
    float2 VelocityUV = getClosestOffset(pin.tex); // sample 3x3 region
    float2 Velocity = GBufferVelocity.Sample(PointClampSampler, VelocityUV).rg;
    if (abs(Velocity.x) > 0.0f || abs(Velocity.y) > 0.0f)
    {
        UVoffset = Velocity;
    }
    
    float3 CurColor = ColorTexture.Sample(PointClampSampler, pin.tex).rgb;
    float3 PreColor = PrevColorTexture.Sample(PointClampSampler, pin.tex - UVoffset).rgb;
    
    CurColor = RGB2YCoCgR(CurColor);
    PreColor = RGB2YCoCgR(PreColor);
    
    
    // Sample neigbhorhoods pixel color of current frame.
    uint SampleCount = 9;
    float3 Moment1 = 0.0f;
    float3 Moment2 = 0.0f;
    
    int x, y, i;
    for (y = -1; y <= 1; ++y)
    {
        for (x = -1; x <= 1; ++x)
        {
            i = (y + 1) * 3 + x + 1;
            
            float2 SampleOffset = float2(x, y) * InvScreenDimensions;
            float2 SampleUV = pin.tex + SampleOffset;
            SampleUV = saturate(SampleUV);
            
            float3 NeighborhoodColor = ColorTexture.Sample(PointClampSampler, SampleUV).rgb;
            
            NeighborhoodColor = RGB2YCoCgR(NeighborhoodColor);
            
            Moment1 += NeighborhoodColor;
            Moment2 += NeighborhoodColor * NeighborhoodColor;
        }
    }

    // Variance clipping
    float3 mean = Moment1 / SampleCount;
    float3 Sigma = sqrt(Moment2 / SampleCount - mean * mean); // standard deviation
    const float VarianceClipGamma = 1.0;
    float3 Min = mean - VarianceClipGamma * Sigma;
    float3 Max = mean + VarianceClipGamma * Sigma;
    
    PreColor = ClipAABB(Min, Max, PreColor);
    
    const float Alpha = 0.1f;
    float3 Color = CurColor * Alpha + PreColor * (1.0f - Alpha);
    
    Color = YCoCgR2RGB(Color);
    
    return float4(Color, 1.0f);
}