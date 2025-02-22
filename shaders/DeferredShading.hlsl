#include "Common.hlsl"
#include "lightInfo.hlsl"
#include "TiledBaseLightCulling.hlsl"

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

StructuredBuffer<TileLightInfo> LightInfoList;


PSInput VSMain(VSInput input)
{
    PSInput vout;
    vout.position = input.position;
    vout.tex = input.tex;
    
    return vout;
}
float2 UVToScreen(float2 UVPos, float2 ScreenSize)
{
    return UVPos * ScreenSize;
}
float4 PSMain(PSInput pin) : SV_Target
{
    // retrieve data from gbuffer
    float3 albedo = GBufferAlbedoMap.Sample(LinearWrapSampler, pin.tex).rgb;
    albedo = pow(albedo, 2.2);
    float3 worldPos = GBufferWorldPosMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float3 normal = GBufferNormalMap.Sample(LinearWrapSampler, pin.tex).rgb;
    float Specular = GBufferSpecularMap.Sample(LinearWrapSampler, pin.tex).r;
    
    float3 N = normalize(normal * 2.0 - 1.0);
    float3 V = normalize(gEyePosW - worldPos);
    
    // Get the tile index of current pixel
    float2 ScreenPos = UVToScreen(pin.tex, ScreenDimensions.xy);
    uint TileX = floor(ScreenPos.x / TILE_BLOCK_SIZE);
    uint TileY = floor(ScreenPos.y / TILE_BLOCK_SIZE);
    uint TileCountX = ceil(ScreenDimensions.x / TILE_BLOCK_SIZE);
    uint TileIndex = TileY * TileCountX + TileX;
		
    TileLightInfo LightInfo = LightInfoList[TileIndex];
    
    // calculate lighting
    float3 totalDiffuse = float3(0.0, 0.0, 0.0);
    float3 totalSpecular = float3(0.0, 0.0, 0.0);
    for (int i = 0; i < LightInfo.LightCount; i++)
    {
        uint LightIndex = LightInfo.LightIndices[i];
        Light light = Lights[LightIndex];
    
        if(light.Type == 1) // directional light
        {
            float3 L = normalize(-light.DirectionW);
        
            float kd = saturate(dot(N, L));
            float3 lightDiffuse = light.Color.rgb * light.Intensity * kd;
        
            float3 H = normalize(V + L);
            float ks = pow(saturate(dot(N, H)), 32);
            float3 lightSpecular = light.Color.rgb * light.Intensity * ks;
        
            totalDiffuse += lightDiffuse;
            totalSpecular += lightSpecular;
        }
        else if(light.Type == 2) // point light
        {
            float3 L = light.PositionW.xyz - worldPos;
            float distance = length(L);
            L = L / distance;
            float attenuation = 1.0 / (distance * distance);
        
            float kd = saturate(dot(N, L));
            float3 lightDiffuse = light.Color.rgb * light.Intensity * kd;
        
            float3 H = normalize(V + L);
            float ks = pow(saturate(dot(N, H)), 32);
            float3 lightSpecular = light.Color.rgb * light.Intensity * ks;
        
            totalDiffuse += lightDiffuse * attenuation;
            totalSpecular += lightSpecular * attenuation;
        }
       
    }
    
    float3 diffuse = totalDiffuse * albedo;
    float3 specular = totalSpecular * Specular;
    
    float3 ambient = albedo * 0.05;
    
    float3 color = diffuse + specular + ambient;
    
    return float4(color, 1.0);
}
