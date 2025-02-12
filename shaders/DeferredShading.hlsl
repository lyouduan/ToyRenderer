
cbuffer passCBuffer
{
    float4x4 gViewMat;
    float4x4 gProjMat;
    
    float3 gEyePosW;
    float gLightIndex;
    
    float3 gLightPos;
    float pad1;
    float3 gLightColor;
    float gIntensity;
}

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

Texture2D GBufferWorldPosMap;
Texture2D GBufferAlbedoMap;
Texture2D GBufferNormalMap;
Texture2D GBufferSpecularMap;

SamplerState PointWrapSampler : register(s0);
SamplerState PointClampSampler : register(s1);
SamplerState LinearWrapSampler : register(s2);
SamplerState LinearClampSampler : register(s3);
SamplerState AnisotropicWrapSampler : register(s4);
SamplerState AnisotropicClampSampler : register(s5);

PSInput VSMain(VSInput input)
{
    PSInput vout;
    vout.position = input.position;
    vout.tex = input.tex;
    
    return vout;
}

float4 PSMain(PSInput pin) : SV_Target
{
    // retrieve data from gbuffer
    float3 albedo = GBufferAlbedoMap.Sample(LinearWrapSampler, pin.tex).rgb;
    albedo = pow(albedo, 2.2);
    float3 worldPos = GBufferWorldPosMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float3 normal = GBufferNormalMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float Specular = GBufferSpecularMap.Sample(LinearWrapSampler, pin.tex).r;
    
    // calculate lighting
    
    float3 color = float3(0.0, 0.0, 0.0);
    
    {
        float3 N = normalize(normal);
        float3 V = normalize(gEyePosW - worldPos);
        float3 R = reflect(-V, N);
    
        float3 L = gLightPos - worldPos;
        float distance = length(L);
        L = L / distance;
        
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = gIntensity * gLightColor;
        
        // bllin-phong shading
        float3 diffuse = saturate(dot(N, L)) * radiance * albedo;
        
        float3 H = normalize(V + L);
        float ks = pow(saturate(dot(N, H)), 32);
        float3 specular = radiance * ks * Specular;
        
        float3 ambient = diffuse * 0.2;
        
        color += diffuse + specular + ambient;
    }
    
    // HDR
    color = color / (color + float3(1.0, 1.0, 1.0));
    // gamma correct
    color = pow(color, 1.0 / 2.2);
    
    return float4(color, 1.0);
}
