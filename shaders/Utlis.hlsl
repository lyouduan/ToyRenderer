#ifndef UTLIS_HLSL
#define UTLIS_HLSL

float4 NDCToView(float4 NDCPos, float4x4 InvProj)
{
    float4 View = mul(NDCPos, InvProj);
    View /= View.w;
 
    return View;
}

float2 ScreenToUV(float2 ScreenPos, float2 ScreenSize)
{
    return ScreenPos /= ScreenSize;
}

float4 UVToNDC(float2 UVPos, float Depth)
{
    return float4(2 * UVPos.x - 1, 1 - 2 * UVPos.y, Depth, 1.0f);
}

float2 NDCToUV(float4 NDCPos)
{
    return float2(0.5 + 0.5 * NDCPos.x, 0.5 - 0.5 * NDCPos.y);
}

float4 ScreenToView(float2 ScreenPos, float2 ScreenSize, float NDCDepth, float4x4 InvProj)
{
    float2 UV = ScreenToUV(ScreenPos, ScreenSize);
	
    float4 NDC = UVToNDC(UV, NDCDepth);
	
    float4 View = NDCToView(NDC, InvProj);
	
    return View;
}

float4 UVToView(float2 UV, float NDCDepth, float4x4 InvProj)
{
    float4 NDC = UVToNDC(UV, NDCDepth);
	
    float4 View = NDCToView(NDC, InvProj);
	
    return View;
}
float4 ViewToNDC(float4 ViewPos, float4x4 Proj)
{
    float4 NDC = mul(ViewPos, Proj);
    NDC /= NDC.w;
	
    return NDC;
}

float2 ViewToUV(float4 ViewPos, float4x4 Proj)
{
    float4 NDC = ViewToNDC(ViewPos, Proj);
	
    float2 UV = NDCToUV(NDC);
	
    return UV;
}

#endif