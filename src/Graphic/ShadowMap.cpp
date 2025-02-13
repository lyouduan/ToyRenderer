#include "ShadowMap.h"

ShadowMap::ShadowMap(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format)
	: m_Width(width)
	, m_Height(height)
	, m_Format(format)
{
	SetViewportAndScissorRect();
	CreateShadowMap(name);
}

ShadowMap::~ShadowMap()
{
}

void ShadowMap::SetLightView(DirectX::XMFLOAT3 lightDir)
{
	DirectX::BoundingSphere mSceneBounds = DirectX::BoundingSphere(DirectX::XMFLOAT3(0.0f, 0.0f, 0.0f), 70.0f);

	// Light Space from directional light
	XMVECTOR lightDirVec = XMLoadFloat3(&lightDir);
	XMVECTOR lightPos = -1.0f * mSceneBounds.Radius * lightDirVec;
	XMVECTOR targetPos = XMLoadFloat3(&mSceneBounds.Center);
	XMVECTOR up = { 0.0f, 1.0f, 0.0f };
	m_LightView = XMMatrixLookAtLH(lightPos, targetPos, up);
	// Transform Sphere to light space
	XMFLOAT3 sphereCenterLS;
	XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, m_LightView));

	float Left   = sphereCenterLS.x - mSceneBounds.Radius;
	float Bottom = sphereCenterLS.y - mSceneBounds.Radius;
	float Near   = sphereCenterLS.z - mSceneBounds.Radius;
	float Right  = sphereCenterLS.x + mSceneBounds.Radius;
	float Top    = sphereCenterLS.y + mSceneBounds.Radius;
	float Far    = sphereCenterLS.z + mSceneBounds.Radius;
	// Ortho Projection
	m_LightProj = XMMatrixOrthographicOffCenterLH(Left, Right, Bottom, Top, Near, Far);

	//Transform NDC space[-1, +1] ^ 2 to texture space[0, 1] ^ 2
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	m_ShadowTransform = m_LightView * m_LightProj * T;
}

void ShadowMap::CreateShadowMap(const std::wstring& name)
{
	m_ShadowMap.Create(name, m_Width, m_Height, m_Format);
}

void ShadowMap::SetViewportAndScissorRect()
{
	m_Viewport.Height = (float)m_Height;
	m_Viewport.Width = (float)m_Width;
	m_Viewport.MinDepth = 0.0;
	m_Viewport.MaxDepth = 1.0;
	m_Viewport.TopLeftX = 0;
	m_Viewport.TopLeftY = 0;

	m_SccisorRect.left = 0;
	m_SccisorRect.top = 0;
	m_SccisorRect.right = m_Width;
	m_SccisorRect.bottom = m_Height;
}
