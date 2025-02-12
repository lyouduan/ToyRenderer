#include "SceneCaptureCube.h"
#include "D3D12CommandContext.h"
#include "D3D12RHI.h"
#include "Shader.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "PSOManager.h"

SceneCaptureCube::SceneCaptureCube(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format)
	: m_Width(Width), m_Height(Height), m_NumMips(NumMips), m_Format(Format)
{
	CubeMap.Create(name, Width, Height, NumMips, Format);

	SetViewportAndScissorRect();

}

SceneCaptureCube::~SceneCaptureCube()
{
	DepthBuffer.Create(L"Depth CubeMap", m_Width, m_Height, DXGI_FORMAT_D32_FLOAT);
}

void SceneCaptureCube::CreateCubeCamera(XMFLOAT3 pos, float nearZ, float farZ)
{
	// dx12 : left hand coordinate
	XMVECTOR Targets[6] =
	{
		{ 1.0,  0.0,  0.0},  // +X
		{-1.0,  0.0,  0.0},  // -X
		{ 0.0,  1.0,  0.0},  // +Y
		{ 0.0, -1.0,  0.0},  // -Y
		{ 0.0,  0.0,  +1.0},  // +Z
		{ 0.0,  0.0,  -1.0},  // -Z
	};

	XMVECTOR Vups[6] =
	{
		{ 0.0,  1.0,  0.0},  // +X
		{ 0.0,  1.0,  0.0},  // -X
		{ 0.0,  0.0,  -1.0},  // +Y
		{ 0.0,  0.0,  1.0},  // -Y
		{ 0.0,  1.0,  0.0},  // +Z
		{ 0.0,  1.0,  0.0},  // -Z
	};

	//XMVECTOR lookat = XMVector3Normalize(XMVectorSubtract(target, pos));
	//XMVECTOR right = XMVector3Normalize(XMVector3Cross(vup, lookat));
	//XMVECTOR up = XMVector3Normalize(XMVector3Cross(lookat, right));

	XMVECTOR posVec = { pos.x, pos.y, pos.z };

	for (int i = 0; i < 6; i++)
	{
		m_Camera[i].SetLookAt(posVec, Targets[i], Vups[i]);
		m_Camera[i].UpdateViewMat();

		float fov = 90; // 45
		float AspectRatio = 1.0; // Sqaure
		m_Camera[i].SetLen(fov, AspectRatio, nearZ, farZ);
	}
}

void SceneCaptureCube::SetViewportAndScissorRect()
{
	m_Viewport = { 0.0, 0.0, (float)m_Width, (float)m_Height, 0.0, 1.0 };
	m_ScissorRect = { 0, 0, (int)m_Viewport.Width, (int)m_Viewport.Height };

	
}
