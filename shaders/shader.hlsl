
cbuffer ModelViewProjectionCB : register(b0)
{
    matrix MVP;
}

struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;
    result.position = mul(MVP, float4(position.xyz, 1.0f));
    result.color = color;
    
    return result;
}

float4 PSMain(PSInput input) : SV_Target
{
    return input.color;
}