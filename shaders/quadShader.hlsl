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

Texture2D tex;

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

float GetActualDepth(float depth, float zNear, float zFar)
{
    // 计算实际深度
    float actualDepth = zNear * zFar / (zFar - depth * (zFar - zNear));
    return actualDepth;
}

float4 PSMain(PSInput pin) : SV_Target
{
    
    // cull light grid debug
    {
        uint2 threadXY = uint2(floor(1280 / 32.0), floor(720 / 32.0));
        uint2 tileIndex = uint2(floor(pin.tex * threadXY));
    
        uint r = LightGrid[tileIndex].g;
        if (r < 5)
            return float4(1.0, 0.0, 0.0, 1.0f);
        if (r < 10)
            return float4(0.0, 1.0, 0.0, 1.0f);
        if (r < 15)
            return float4(0.0, 0.0, 1.0, 1.0f);
        if (r < 20)
            return float4(1.0, 0.0, 1.0, 1.0f);
        else
            return float4(1.0, 1.0, 1.0, 1.0f);
    }
    
    //{
    //    float3 color = tex.Sample(PointClampSampler, pin.tex).rgb;
    //    return float4(color, 1.0);
    //
    //}
}