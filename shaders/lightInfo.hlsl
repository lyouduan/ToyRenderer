#ifndef LIGHTINFO
#define LIGHTINFO

#ifndef NUM_LIGHTS
#pragma message("NUM_LIGHTS undefined. Default to 10.")
#define NUM_LIGHTS 10 // should be defined by the application.
#endif

struct Light
{
    float4 PositionW; // position for point and spot lights in World Space
    float4 DirectionW; // direction for spot and directional lights in World Space
    float4 PositionV; // position for point and spot lights in View space
    float4 DirectionV; // direction for spot and directional lights in View space
    float4 Color;
    
    float SpotlightAngle; // the half of the spotlight cone
    float Range; // the range of the light
    float Intensity; // the intensity of the light
    float pad0; // the padding of alignment;
    
    uint  Type;
};

StructuredBuffer<Light> Lights;

#endif // !LIGHTINFO