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

float4 PSMain(PSInput pin) : SV_Target
{
    // cull light grid debug
    {
        uint2 threadXY = uint2(floor(1280 / 16.0), floor(720 / 16.0));
        uint2 tileIndex = uint2(floor(pin.tex * threadXY));
    
        uint r = LightGrid[tileIndex].g;
        if (r < 0)
            return float4(0.0, 0.0, 0.0, 1.0f);
        if (r < 10)
            return float4(0.5, 0.5, 0.5, 1.0f);
        if (r < 20)
            return float4(1.0, 0.0, 0.0, 1.0f);
        if (r < 40)
            return float4(0.0, 1.0, 0.0, 1.0f);
        if (r < 60)
            return float4(0.0, 0.0, 1.0, 1.0f);
        if (r < 80)
            return float4(1.0, 0.0, 1.0, 1.0f);
        else
            return float4(1.0, 1.0, 1.0, 1.0f);
    }
}