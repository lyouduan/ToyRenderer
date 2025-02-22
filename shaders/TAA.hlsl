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
    
    const float Alpha = 0.1f;
    float3 Color = CurColor * Alpha + PreColor * (1.0f - Alpha);
    
    return float4(Color, 1.0f);
}