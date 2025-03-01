#include "Common.hlsl"

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

TextureCube CubeMap : register(t0);

PSInput VSMain(VSInput vin)
{
    PSInput vout;
    
    // Use local vertex position as cubemap lookup vector.
    vout.positionW = vin.position.xyz;
    
    //float4 posW = mul(float4(vin.position.xyz, 1.0f), ModelMat);
    
    // Remove translation from the view matrix
    float4x4 View = gViewMat;
    View[3][0] = View[3][1] = View[3][2] = 0.0f;
    
    vout.position = mul(mul(float4(vin.position.xyz, 1.0), View), gProjMat).xyww;
    
    return vout;
}

StructuredBuffer<float3> SH_Coefficients;

static const float SHBasis[9] =
{
    0.28209479f,
    0.48860251f,
    0.48860251f,
    0.48860251f,
    1.09254843f,
    1.09254843f,
    0.31539157f,
    1.09254843f,
    0.54627421f,
};

float3 GetSHBasis(int i, float3 N)
{
    
    
    switch (i)
    {
        case 0:
            return SHBasis[0];
        case 1:
            return SHBasis[1] * N.y;
        case 2:
            return SHBasis[2] * N.z;
        case 3:
            return SHBasis[3] * N.x;
        case 4:
            return SHBasis[4] * N.x * N.y;
        case 5:
            return SHBasis[5] * N.y * N.z;
        case 6:
            return SHBasis[6] * (-N.x * N.x - N.y * N.y + 2 * N.z * N.z);
        case 7:
            return SHBasis[7] * (N.z * N.x);
        case 8:
            return SHBasis[8] * (N.x * N.x - N.y * N.y);
        default:
            return 0.0f;
    }
}

float4 PSMain(PSInput pin) : SV_Target
{
    float3 N = normalize(pin.positionW);
    
    float3 color = CubeMap.SampleLevel(LinearWrapSampler, N, 0.0).rgb;
    
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, 1.0 / 2.2);
    
    //float3 diffuse = float3(0.0, 0.0, 0.0);
    //for (int i = 0; i < 9; i++)
    //{
    //    diffuse += SH_Coefficients[i] * GetSHBasis(i, N);
    //}
    
    return float4(color, 1.0);
}