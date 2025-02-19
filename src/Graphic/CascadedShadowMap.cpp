#include "CascadedShadowMap.h"
#include "Display.h"
#include "Camera.h"
#include <limits>
#include "ImGuiManager.h"

CascadedShadowMap::CascadedShadowMap(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t count)
	: m_Width(width), m_Height(height), m_Format(format), m_CascadeCount(count)
{
	SetViewportAndScissorRect();

	CreateShadowMaps(name);
	m_FrustumVSFarZ.resize(m_CascadeCount);
	m_LightProj.resize(m_CascadeCount);
	m_LightView.resize(m_CascadeCount);
	m_ShadowTransform.resize(m_CascadeCount);
}

CascadedShadowMap::~CascadedShadowMap()
{
	m_ShadowMaps.clear();

	m_FrustumVSFarZ.clear();
	m_LightProj.clear();
	m_LightView.clear();
	m_ShadowTransform.clear();

}

void CascadedShadowMap::DivideFrustum(DirectX::XMFLOAT3 lightDir, float zNear, float zFar)
{
	//Transform NDC space[-1, +1] ^ 2 to texture space[0, 1] ^ 2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	float zLength = zFar - zNear;
	float Scale[8] = { 0.1f, 0.2, 0.4f, 2.0f, 1.0f, 1.0f, 1.0f, 1.0f };

	float weights = 0.0f;
	for (UINT i = 0; i < m_CascadeCount; i++)
	{
		weights += Scale[i];
	}
	weights = 1.0f / weights;

	float percentage = 0.0f;

	for (UINT i = 0; i < m_CascadeCount; i++)
	{
		// 计算每一级 cascade 的百分比例值
		float percentageOffset = Scale[i] * weights;

		// 将比例值映射到 Frustum，从而求出各级 SubFrustum 的 zNear 和 zFar
		float zCascadeNear = zNear + percentage * zLength;
		percentage += percentageOffset;
		float zCascadeFar = zNear + percentage * zLength;
		m_FrustumVSFarZ[i] = XMFLOAT2(zCascadeNear, zCascadeFar);


		// Get World Space Frustum 
		auto camera = TD3D12RHI::g_Camera;
		XMMATRIX ProjMat = XMMatrixPerspectiveFovLH(XMConvertToRadians(camera.GetFovY()), camera.GetAspect(), zCascadeNear, zCascadeFar);
		 auto frustumCorners = GetFrustumCorners(camera.GetViewMat(), ProjMat);
		
		//XMVECTOR center = XMVectorSet(0.0, 0.0, 0.0, 0.0);
		//for (const auto& v : frustumCorners)
		//	center += v;
		//// 求视锥体中心
		//float size = frustumCorners.size();
		//center /= size;

		const auto LightView = XMMatrixLookAtLH(- XMLoadFloat3(&lightDir), XMVectorSet(0.0, 0.0, 0.0, 0.0), XMVectorSet(0.0, 1.0, 0.0, 0.0));

		float minX = FLT_MAX;
		float maxX = FLT_MIN;
		float minY = FLT_MAX;
		float maxY = FLT_MIN;
		float minZ = FLT_MAX;
		float maxZ = FLT_MIN;

		for (int i = 0; i < frustumCorners.size(); i++)
		{
			frustumCorners[i] = XMVector4Transform(frustumCorners[i], LightView);
			minX = min(minX, frustumCorners[i][0]);
			maxX = max(maxX, frustumCorners[i][0]);
			minY = min(minY, frustumCorners[i][1]);
			maxY = max(maxY, frustumCorners[i][1]);
			minZ = min(minZ, frustumCorners[i][2]);
			maxZ = max(maxZ, frustumCorners[i][2]);
		}

		XMVECTOR vLightCameraOrthographicMax = XMVectorSet(maxX, maxY, maxZ, 1.0f);
		XMVECTOR vLightCameraOrthographicMin = XMVectorSet(minX, minY, minZ, 1.0f);

		XMVECTOR vDiagonal = frustumCorners[4] - frustumCorners[7];
		XMVECTOR vCrossDiagonal = frustumCorners[0] - frustumCorners[4];
		vDiagonal = XMVector3Length(vDiagonal);
		vCrossDiagonal = XMVector3Length(vCrossDiagonal);
		FLOAT farDist =XMVectorGetX(vDiagonal);
		FLOAT crossDist =XMVectorGetX(vCrossDiagonal);

		float maxDist = farDist > crossDist ? farDist : crossDist;

		XMVECTOR vBoarderOffset = (maxDist - (vLightCameraOrthographicMax - vLightCameraOrthographicMin)) * 0.5f;

		vBoarderOffset *= XMVectorSet(1.0, 1.0, 0.0f, 0.0f);

		vLightCameraOrthographicMax += vBoarderOffset;
		vLightCameraOrthographicMin -= vBoarderOffset;

		FLOAT fWorldUnitsPerTexel = maxDist / (float)m_Width;
		XMVECTOR vWorldUnitsPerTexel = XMVectorSet(fWorldUnitsPerTexel, fWorldUnitsPerTexel, 0.0f, 0.0f);
	
		vLightCameraOrthographicMin /= vWorldUnitsPerTexel;
		vLightCameraOrthographicMin = XMVectorFloor(vLightCameraOrthographicMin);
		vLightCameraOrthographicMin *= vWorldUnitsPerTexel;

		vLightCameraOrthographicMax /= vWorldUnitsPerTexel;
		vLightCameraOrthographicMax = XMVectorFloor(vLightCameraOrthographicMax);
		vLightCameraOrthographicMax *= vWorldUnitsPerTexel;
		

		const XMMATRIX lightOrthProj = XMMatrixOrthographicOffCenterLH(XMVectorGetX(vLightCameraOrthographicMin), XMVectorGetX(vLightCameraOrthographicMax),
			XMVectorGetY(vLightCameraOrthographicMin), XMVectorGetY(vLightCameraOrthographicMax), 
			minZ, maxZ);
		//const XMMATRIX lightOrthProj = XMMatrixOrthographicLH(maxX-minX, maxY-minY, minZ, maxZ);

		m_LightView[i] = LightView;
		m_LightProj[i] = lightOrthProj;
		m_ShadowTransform[i] = LightView * lightOrthProj * T;
	}
}

void CascadedShadowMap::CreateShadowMaps(const std::wstring& name)
{
	for(int i = 0; i < m_CascadeCount; i++)
	{
		D3D12DepthBuffer shadowMap;
		shadowMap.Create(name + std::to_wstring(i), m_Width, m_Height, DXGI_FORMAT_D32_FLOAT);
		m_ShadowMaps.push_back(shadowMap);
	}
}

void CascadedShadowMap::SetViewportAndScissorRect()
{
	m_Viewport.TopLeftX = 0.0f;
	m_Viewport.TopLeftY = 0.0f;
	m_Viewport.Width = static_cast<float>(m_Width);
	m_Viewport.Height = static_cast<float>(m_Height);
	m_Viewport.MinDepth = 0.0f;
	m_Viewport.MaxDepth = 1.0f;
	m_SccisorRect.left = 0;
	m_SccisorRect.top = 0;
	m_SccisorRect.right = static_cast<LONG>(m_Width);
	m_SccisorRect.bottom = static_cast<LONG>(m_Height);
}

std::vector<XMVECTOR> CascadedShadowMap::GetFrustumCorners(DirectX::XMMATRIX camera_view, DirectX::XMMATRIX camera_proj)
{
	auto viewProj = camera_view * camera_proj;
	XMVECTOR det;
	const XMMATRIX invViewProj = XMMatrixInverse(&det, viewProj);

	std::vector<XMVECTOR> frustumCorners;

	XMVECTOR nearPlane[4];
	XMVECTOR farPlane[4];
	nearPlane[0] = XMVector4Transform(XMVectorSet(-1.0f, 1.0f, 0.0f, 1.0f), invViewProj);
	nearPlane[1] = XMVector4Transform(XMVectorSet( 1.0f, 1.0f, 0.0f, 1.0f), invViewProj);
	nearPlane[2] = XMVector4Transform(XMVectorSet(-1.0f,-1.0f, 0.0f, 1.0f), invViewProj);
	nearPlane[3] = XMVector4Transform(XMVectorSet( 1.0f,-1.0f, 0.0f, 1.0f), invViewProj);
	farPlane[0]  = XMVector4Transform(XMVectorSet(-1.0f, 1.0f, 1.0f, 1.0f), invViewProj);
	farPlane[1]  = XMVector4Transform(XMVectorSet( 1.0f, 1.0f, 1.0f, 1.0f), invViewProj);
	farPlane[2]  = XMVector4Transform(XMVectorSet(-1.0f,-1.0f, 1.0f, 1.0f), invViewProj);
	farPlane[3]  = XMVector4Transform(XMVectorSet( 1.0f,-1.0f, 1.0f, 1.0f), invViewProj);

	for (unsigned int i = 0; i < 4; ++i)
	{
		nearPlane[i] /= nearPlane[i][3];
		frustumCorners.push_back(nearPlane[i]);
	}

	for (unsigned int i = 0; i < 4; ++i)
	{
		farPlane[i] /= farPlane[i][3];
		frustumCorners.push_back(farPlane[i]);
	}

	return frustumCorners;
}
