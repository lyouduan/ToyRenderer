#pragma once
#include <DirectXMath.h>
#include "D3D12Buffer.h"
#include "RenderInfo.h"

using namespace DirectX;

enum ELightType
{
	None,
	DirectionalLight,
	PointLight,
	SpotLight,
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

    void SetLightInfo(LightInfo lightinfo)
    {
        std::vector<LightInfo> infos;
        infos.push_back(lightinfo);
        SetLightInfo(infos);
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
    extern Light DirectionalLight;

    void InitialzeLights();
}