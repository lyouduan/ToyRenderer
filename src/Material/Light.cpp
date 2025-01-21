#include "Light.h"
#include "D3D12RHI.h"
#include <assert.h>
#include "Camera.h"
#include "Display.h"

void Light::CreateStructuredBufferRef()
{
	assert(!m_lights.empty() && "LightInfo is not nullptr");

	m_StruturedBuffer = TD3D12RHI::CreateStructuredBuffer(m_lights.data(), sizeof(LightInfo), m_lights.size());
}

namespace LightManager
{
	Light g_light;

	void InitialzeLights()
	{
		// point light
		std::vector<LightInfo> lights;

		LightInfo light;
		light.Type = ELightType::SpotLight;
		for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < 5; j++)
			{

				light.Color = XMFLOAT4{ 1.0, 0.0, 0.0, 1.0 };
				light.PositionW = XMFLOAT4{ (float)i * 10.0f,  -50.0f + 20 * (float)i, 10.0f * (float)j, 1.0 };

				XMVECTOR pos = XMLoadFloat4(&light.PositionW);
				XMStoreFloat4(&light.PositionV, XMVector4Transform(pos, TD3D12RHI::g_Camera.GetViewMat()));

				light.Intensity = 50 + i * j * 20;
				light.Range = 10 + i * j * 5;

				lights.push_back(light);
			}
		}

		g_light.SetLightInfo(lights);
		g_light.CreateStructuredBufferRef();

	}
}


