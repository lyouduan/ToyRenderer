Texture2D<float2> InputTexture;
RWTexture2D<float2> OutputTexture;

cbuffer BlurCBuffer
{
    int gBlurRadius;
    
	// Support up to 11 blur weights.
    float w0;
    float w1;
    float w2;
    float w3;
    
    float w4;
    float w5;
    float w6;
    float w7;
    
    float w8;
    float w9;
    float w10;
};


#define N 256
#define MAX_BLUR_RADIUS 5
#define CacheSize (N + 2 * MAX_BLUR_RADIUS)
groupshared float2 gCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // 填入本地线程存储区，减少带宽负载
    // 对N个像素模糊，需要R + N + R个像素
    
    // 处理左边界
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        gCache[groupThreadID.x] = InputTexture[int2(x, dispatchThreadID.y)];
    }
    // 处理右边界
    if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(dispatchThreadID.x + gBlurRadius, InputTexture.Length.x - 1);
        gCache[groupThreadID.x + 2 * gBlurRadius] = InputTexture[int2(x, dispatchThreadID.y)];
    }
    // 正常范围，偏移gBlurRadius个像素，同时防止图像边界越界
    gCache[groupThreadID.x + gBlurRadius] = InputTexture[min(dispatchThreadID.xy, InputTexture.Length.xy - 1)];
    
    // 等待所有线程完成
    GroupMemoryBarrierWithGroupSync();
    
    // 处理每个像素
    float2 blurColor = float2(0, 0);
    
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.x + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
    
    OutputTexture[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID,
				int3 dispatchThreadID : SV_DispatchThreadID)
{
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // 填入本地线程存储区，减少带宽负载
    // 对N个像素模糊，需要R + N + R个像素
    
    // 处理左边界
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        gCache[groupThreadID.y] = InputTexture[int2(dispatchThreadID.x, y)];
    }
    // 处理右边界
    if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(dispatchThreadID.y + gBlurRadius, InputTexture.Length.y - 1);
        gCache[groupThreadID.y + 2 * gBlurRadius] = InputTexture[int2(dispatchThreadID.x, y)];
    }
    // 正常范围，偏移gBlurRadius个像素，同时防止图像边界越界
    gCache[groupThreadID.y + gBlurRadius] = InputTexture[min(dispatchThreadID.xy, InputTexture.Length.xy - 1)];
    
    // 等待所有线程完成
    GroupMemoryBarrierWithGroupSync();
    
    // 处理每个像素
    float2 blurColor = float2(0, 0);
    
    for (int i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
        int k = groupThreadID.y + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
    
    OutputTexture[dispatchThreadID.xy] = blurColor;
}