#define NUM_THREADS 16

Texture2D<float> ShadowMap;
RWTexture2D<float2> VSM;
RWTexture2D<float> ESM;
RWTexture2D<float4> EVSM;

[numthreads(NUM_THREADS, NUM_THREADS, 1)]
void CS(int3 dispatchThreadID : SV_DispatchThreadID)
{
#if USE_VSM 
    float Depth = ShadowMap[dispatchThreadID.xy];
    VSM[dispatchThreadID.xy] = float2(Depth, Depth * Depth);
#elif USE_ESM 
    float Depth = ShadowMap[dispatchThreadID.xy];
    ESM[dispatchThreadID.xy] = exp(60 * Depth);
#elif USE_EVSM    
    float Depth = ShadowMap[dispatchThreadID.xy];
    float e1 = exp(60 * Depth);
    float e2 = -exp(-60 * Depth);
    EVSM[dispatchThreadID.xy] = float4(e1, e1 * e1, e2, e2 * e2);
#endif
}   