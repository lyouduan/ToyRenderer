#pragma once
#include <DirectXMath.h>
#include "D3D12Buffer.h"

using namespace DirectX;

enum ELightType
{
	None,
	DirectionalLight,
	PointLight,
	SpotLight,
};

struct LightInfo
{
    XMFLOAT4 PositionW;
    XMFLOAT4 DirectionW;
    XMFLOAT4 PositionV;
    XMFLOAT4 DirectionV;
    XMFLOAT4 Color;

    float SpotlightAngle;
    float Range;
    float Intensity;
    float pad0;

    XMFLOAT4X4 ModelMat = MATH::IdentityMatrix;

    int Type;
};

class Light
{
public:

    Light(){}
    ~Light(){}

    void SetLightInfo(std::vector<LightInfo>& lights)
    {
        m_lights = lights;
    }

    std::vector<LightInfo> GetLightInfo()
    {
        return m_lights;
    }

    TD3D12StructuredBufferRef GetStructuredBuffer()
    {
        return m_StruturedBuffer;
    }

    void CreateStructuredBufferRef();

private:

    std::vector<LightInfo> m_lights;

    TD3D12StructuredBufferRef m_StruturedBuffer = nullptr;
};

namespace LightManager
{
    extern Light g_light;

    void InitialzeLights();
}