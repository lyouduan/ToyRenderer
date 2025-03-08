#include "Common.hlsl"
#include "Utlis.hlsl"

Texture2D ColorTexture;
Texture2D PrevColorTexture;
Texture2D GBufferVelocity;
Texture2D GbufferDepth;
Texture2D PrevGbufferDepth;

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
    float2 closestuv = tex;


    
    for (int i = -1; i <= 1; ++i)
    {
        for (int j = -1; j <= 1; ++j)
        {
            float2 newuv = tex + float2(i, j) * InvScreenDimensions;

            float depth = GbufferDepth.Sample(PointClampSampler, newuv).r;
            if (depth < closestDepth)
            {
                closestDepth = depth;
                closestuv = newuv;
            }
        }
    }

    return closestuv;
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

cbuffer TAASettings
{
    float2 Jitter;
};
float4 CatmullRomSharpen(Texture2D tex, SamplerState samp, float2 uv, float2 texelSize)
{
    // 计算采样坐标
    float2 pos = uv * texelSize - 0.5; // 转换到 texel 空间
    float2 fractPart = frac(pos); // 获取小数部分
    float2 baseCoord = floor(pos) * texelSize;

    // 采样 4 个相邻像素
    float4 P0 = tex.SampleLevel(samp, baseCoord - texelSize, 0);
    float4 P1 = tex.SampleLevel(samp, baseCoord, 0);
    float4 P2 = tex.SampleLevel(samp, baseCoord + texelSize, 0);
    float4 P3 = tex.SampleLevel(samp, baseCoord + 2.0 * texelSize, 0);

    // 使用 Catmull-Rom 进行插值（权重可以调整锐度）
    float t = fractPart.x;
    float4 result = 0.5 * (
        2.0 * P1 +
        (-P0 + P2) * t +
        (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * t * t +
        (-P0 + 3.0 * P1 - 3.0 * P2 + P3) * t * t * t
    );

    return result;
}

float4 PS(PSInput pin) : SV_Target
{
	//Compute uvoffset to last frame
    float2 uvoffset;
    
    float2 uv = pin.tex;
    
    float2 Velocityuv = getClosestOffset(uv); // sample 3x3 region
    float2 Velocity = GBufferVelocity.Sample(LinearClampSampler, uv).rg;
    if (abs(Velocity.x) > 0.0f || abs(Velocity.y) > 0.0f)
    {
        uvoffset = Velocity;
    }
    
    float2 lastuv = uv - uvoffset;
    
    float Depth = GbufferDepth.Sample(LinearClampSampler, uv).r;
    float prevDepth = PrevGbufferDepth.Sample(LinearClampSampler, lastuv).r;
    
    float3 CurColor = ColorTexture.Sample(PointClampSampler, uv).rgb;
    float4 PreColor = PrevColorTexture.Sample(PointClampSampler, lastuv).rgba;
    
    CurColor = RGB2YCoCgR(CurColor);
    PreColor.rgb = RGB2YCoCgR(PreColor.rgb);
    
    
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
            float2 Sampleuv = uv + SampleOffset;
            Sampleuv = saturate(Sampleuv);
            
            float3 NeighborhoodColor = ColorTexture.Sample(LinearClampSampler, Sampleuv).rgb;
            
            NeighborhoodColor = RGB2YCoCgR(NeighborhoodColor);
            
            Moment1 += NeighborhoodColor;
            Moment2 += NeighborhoodColor * NeighborhoodColor;
        }
    }

    if (PreColor.a < 100)
        PreColor.a = PreColor.a + 1.0;
    
    // Variance clipping
    float3 mean = Moment1 / SampleCount;
    float3 Sigma = sqrt(Moment2 / SampleCount - mean * mean); // standard deviation
    const float VarianceClipGamma = 1.0;
    float3 Min = mean - VarianceClipGamma * Sigma;
    float3 Max = mean + VarianceClipGamma * Sigma;
    
    float3 clampPreColor = ClipAABB(Min, Max, PreColor.rgb);
    
    float3 diff = abs(clampPreColor - PreColor.rgb);
    if (any(diff > 0.013))
    {
        PreColor.a = 0.0;
    }
    
    //const float weightofHistoryD = 0.9f;
    float t = max(0, 1 - PreColor.a / 100);
    float weightofHistoryD = lerp(0.95, 0.99, t);

    //float3 depthDiff = abs(Depth - prevDepth);
    //float depthAdaptiveForce = depthDiff < 0.05 ? 0.99 : 0.05;
    
    float3 Color = lerp(CurColor, clampPreColor, weightofHistoryD);
    
    Color = YCoCgR2RGB(Color);
    
    return float4(Color, PreColor.a);
}