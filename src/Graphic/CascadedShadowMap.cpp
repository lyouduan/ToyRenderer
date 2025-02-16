#include "CascadedShadowMap.h"
#include "Display.h"
#include "Camera.h"
#include <limits>



CascadedShadowMap::CascadedShadowMap(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t count)
	: m_Width(width), m_Height(height), m_Format(format), m_CascadeCount(count)
{
	SetViewportAndScissorRect();

	CreateShadowMaps(name);
}

CascadedShadowMap::~CascadedShadowMap()
{
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
	float expScale[8] = { 0.1f, 0.2f, 0.3f, 0.4f, 1.0f, 1.0f, 1.0f, 1.0f };
	float m_cascadeExponentScale = 1.5f;

	float expNormalizeFactor = 0.0f;
	for (UINT i = 1; i < m_CascadeCount; i++)
	{
		expScale[i] = expScale[i - 1] * m_cascadeExponentScale;
		expNormalizeFactor += expScale[i];
	}
	expNormalizeFactor = 1.0f / expNormalizeFactor;

	float percentage = 0.0f;

	m_FrustumVSFarZ.clear();
	m_LightProj.clear();
	m_LightView.clear();

	for (UINT i = 0; i < m_CascadeCount; i++)
	{
		// 计算每一级 cascade 的百分比例值
		float percentageOffset = expScale[i] * expNormalizeFactor;

		// 将比例值映射到 Frustum，从而求出各级 SubFrustum 的 zNear 和 zFar
		float zCascadeNear = zNear + percentage * zLength;
		percentage += percentageOffset;
		float zCascadeFar = zNear + percentage * zLength;
		m_FrustumVSFarZ.push_back(zCascadeFar);

		// Get World Space Frustum 
		auto camera = TD3D12RHI::g_Camera;
		XMMATRIX ProjMat = XMMatrixPerspectiveFovLH(45.0f, camera.GetAspect(), zCascadeNear, zCascadeFar);
		const auto frustumCorners = GetFrustumCorners(camera.GetViewMat(), ProjMat);
		XMVECTOR center = XMVectorSet(0.0, 0.0, 0.0, 0.0);
		for (const auto& v : frustumCorners)
			center += v;
		// 求视锥体中心
		float size = frustumCorners.size();
		center /= size;
		const auto LightView = XMMatrixLookAtLH(center - XMLoadFloat3(&lightDir), center, XMVectorSet(0.0, 1.0, 0.0, 0.0));


		float minX = FLT_MAX;
		float maxX = FLT_MIN;
		float minY = FLT_MAX;
		float maxY = FLT_MIN;
		float minZ = FLT_MAX;
		float maxZ = FLT_MIN;

		for (const auto& v : frustumCorners)
		{
			auto trf = XMVector4Transform(v, LightView);
			minX = min(minX, trf[0]);
			maxX = max(maxX, trf[0]);
			minY = min(minY, trf[1]);
			maxY = max(maxY, trf[1]);
			minZ = min(minZ, trf[2]);
			maxZ = max(maxZ, trf[2]);
		}
		// Tune this parameter according to the scene
		constexpr float zMult = 5.0f;
		if (minZ < 0)
		{
			minZ *= zMult;
		}
		else
		{
			minZ /= zMult;
		}

		const XMMATRIX lightOrthProj = XMMatrixOrthographicOffCenterLH(minX * 1.5, maxX * 1.5, minY * 1.5, maxY * 1.5, minZ, maxZ);
		// Ortho Projection

		m_LightView.push_back(LightView);
		m_LightProj.push_back(lightOrthProj);
		m_ShadowTransform.push_back(LightView * lightOrthProj * T);
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
	auto det = XMMatrixDeterminant(viewProj);
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

		farPlane[i] /= farPlane[i][3];
		frustumCorners.push_back(farPlane[i]);
	}

	return frustumCorners;
}
