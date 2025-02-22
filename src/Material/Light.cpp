#include "Light.h"
#include "D3D12RHI.h"
#include <assert.h>
#include "Camera.h"
#include "Display.h"
#include "ImGuiManager.h"

void Light::CreateStructuredBufferRef()
{
	assert(!m_lights.empty() && "LightInfo is not nullptr");

	m_StruturedBuffer = TD3D12RHI::CreateStructuredBuffer(m_lights.data(), sizeof(LightInfo), m_lights.size());
}

namespace LightManager
{
	Light g_light;
	Light DirectionalLight;

	void InitialzeLights()
	{
		
		// point light
		std::vector<LightInfo> lights;

		LightInfo light;
		light.Type = (int)ELightType::PointLight;
		std::srand(std::time(0));
		for (int i = 1; i < 10; i++)
		{

			for (int j = 1; j < 10; j++)
			{

				light.Color = XMFLOAT4{ static_cast<float>(std::rand()) / RAND_MAX, static_cast<float>(std::rand()) / RAND_MAX ,static_cast<float>(std::rand())/RAND_MAX, 1.0 };
				light.PositionW = XMFLOAT4{ -50.0f + (float)i * 10.0f,  1, -50.0f + 10.0f * (float)j, 1.0 };

				XMMATRIX tanslation = XMMatrixTranslation(light.PositionW.x, light.PositionW.y, light.PositionW.z);
				XMStoreFloat4x4(&light.ModelMat, XMMatrixTranspose(tanslation));

				light.Intensity = 50;
				light.Range = 20;

				lights.push_back(light);
			}
		}

		XMFLOAT3 lightDir = XMFLOAT3{ 10.0f, -10.0f, 10.0f };
		// directional light
		LightInfo directionalLight;
		directionalLight.Type = (int)ELightType::DirectionalLight;
		directionalLight.Color = XMFLOAT4{ 1.0, 1.0, 1.0, 1.0 };
		directionalLight.DirectionW = XMFLOAT4{ lightDir.x,lightDir.y,lightDir.z, 1.0f };
		directionalLight.Intensity = 2;
		lights.push_back(directionalLight);

		g_light.SetLightInfo(lights);
		g_light.CreateStructuredBufferRef();

	}
}


