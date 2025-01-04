#include "SceneCaptureCube.h"
#include "D3D12CommandContext.h"
#include "D3D12RHI.h"
#include "Shader.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "PSOManager.h"

SceneCaptureCube::SceneCaptureCube(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format)
	: m_Wdith(Width), m_Height(Height), m_NumMips(NumMips), m_Format(Format)
{
	CubeMap.Create(name, Width, Height, NumMips, Format);
	DepthBuffer.Create(L"Depth CubeMap", Width, Height, DXGI_FORMAT_D32_FLOAT);

	SetViewportAndScissorRect();

}

SceneCaptureCube::~SceneCaptureCube()
{

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

void SceneCaptureCube::DrawEquirectangularMapToCubeMap(TD3D12CommandContext& gfxContext)
{
	auto shader = PSOManager::m_shaderMap["cubemapShader"];
	auto pso = PSOManager::m_gfxPSOMap["cubemapPSO"];
	auto boxMesh = ModelManager::m_MeshMaps["box"];

	// reset CommandList
	gfxContext.ResetCommandList();

	// set viewport and scirssor
	gfxContext.GetCommandList()->RSSetViewports(1, &m_Viewport);
	gfxContext.GetCommandList()->RSSetScissorRects(1, &m_ScissorRect);

	// set RootSignature and PSO
	gfxContext.GetCommandList()->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxContext.GetCommandList()->SetPipelineState(pso.GetPSO());

	// transition buffer state
	gfxContext.Transition(GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	gfxContext.Transition(DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	ObjCBuffer obj;
	PassCBuffer passcb;

	auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

	XMStoreFloat4x4(&passcb.ProjMat, XMMatrixTranspose(m_Camera[0].GetProjMat()));

	for (UINT i = 0; i < 6; i++)
	{
		float clearColor[4] = { 0.0,0.0,0.0,0.0 };
		gfxContext.GetCommandList()->ClearRenderTargetView(CubeMap.GetRTV(i), (FLOAT*)clearColor, 0, nullptr);
		gfxContext.GetCommandList()->ClearDepthStencilView(DepthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

		gfxContext.GetCommandList()->OMSetRenderTargets(1, &CubeMap.GetRTV(i), TRUE, &DepthBuffer.GetDSV());
		
		XMStoreFloat4x4(&passcb.ViewMat, XMMatrixTranspose(m_Camera[i].GetViewMat()));
		auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));

		shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
		shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
		shader.SetParameter("equirectangularMap", TextureManager::m_SrvMaps["loft"]);
		shader.SetDescriptorCache(boxMesh.GetTD3D12DescriptorCache());
		shader.BindParameters();

		boxMesh.DrawMesh(gfxContext);
	}

	// transition buffer state
	gfxContext.Transition(GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	gfxContext.Transition(DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);

	gfxContext.ExecuteCommandLists();
}


void SceneCaptureCube::SetViewportAndScissorRect()
{
	m_Viewport = { 0.0, 0.0, (float)m_Wdith, (float)m_Height, 0.0, 1.0 };
	m_ScissorRect = { 0, 0, (int)m_Viewport.Width, (int)m_Viewport.Height };
}
