#ifndef LIGHTINFO
#define LIGHTINFO

#ifndef NUM_LIGHTS
#pragma message("NUM_LIGHTS undefined. Default to 81.")
#define NUM_LIGHTS 82 // should be defined by the application.
#endif

struct Light
{
    float4 PositionW; // position for point and spot lights in World Space
    float4 DirectionW; // direction for spot and directional lights in World Space
    float4 Color;
    
    float SpotlightAngle; // the half of the spotlight cone
    float Range; // the range of the light
    float Intensity; // the intensity of the light
    float pad0; // the padding of alignment;
    
    float4x4 ModelMat;
    float4x4 ShadowTransform;
    float4x4 CSMTransform[4];
    uint  Type;
};

StructuredBuffer<Light> Lights;

#endif // !LIGHTINFO