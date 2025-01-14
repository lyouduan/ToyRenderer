#include "Render.h"
#include "D3D12RHI.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "ModelManager.h"

using namespace TD3D12RHI;

TRender::TRender()
{
}

TRender::~TRender()
{
}

void TRender::Initalize()
{
	CreateSceneCaptureCube();
}

void TRender::CreateIBLEnvironmentMap()
{
	g_CommandContext.ResetCommandList();

	auto shader = PSOManager::m_shaderMap["cubemapShader"];
	auto pso = PSOManager::m_gfxPSOMap["cubemapPSO"];
	auto boxMesh = ModelManager::m_MeshMaps["box"];
	auto texSRV = TextureManager::m_SrvMaps["loft"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	g_CommandContext.Transition(IBLEnvironmentMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	gfxCmdList->RSSetViewports(1, IBLEnvironmentMap->GetViewport());
	gfxCmdList->RSSetScissorRects(1, IBLEnvironmentMap->GetScissorRect());

	// set viewport and scirssor
	// set RootSignature and PSO
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	ObjCBuffer obj;
	PassCBuffer passcb;

	for (UINT i = 0; i < 6; i++)
	{
		float clearColor[4] = { 0.0,0.0,0.0,0.0 };
		gfxCmdList->ClearRenderTargetView(IBLEnvironmentMap->GetRTV(i), (FLOAT*)clearColor, 0, nullptr);
		//gfxCmdList->ClearDepthStencilView(IBLIrradianceMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
		gfxCmdList->OMSetRenderTargets(1, &IBLEnvironmentMap->GetRTV(i), TRUE, nullptr);

		// update CBuffer
		{
			auto camera = IBLIrradianceMap->GetCamera(i);
			XMStoreFloat4x4(&passcb.ViewMat, XMMatrixTranspose(camera.GetViewMat()));
			XMStoreFloat4x4(&passcb.ProjMat, XMMatrixTranspose(camera.GetProjMat()));
		}
		auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
		auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

		shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
		shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
		shader.SetParameter("equirectangularMap", texSRV);
		shader.SetDescriptorCache(boxMesh.GetTD3D12DescriptorCache());
		shader.BindParameters();
		boxMesh.DrawMesh(g_CommandContext);
	}

	// transition buffer state
	g_CommandContext.Transition(IBLEnvironmentMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::CreateIBLIrradianceMap()
{
	g_CommandContext.ResetCommandList();

	auto shader = PSOManager::m_shaderMap["irradianceMapShader"];
	auto pso = PSOManager::m_gfxPSOMap["irradianceMapPSO"];
	auto boxMesh = ModelManager::m_MeshMaps["box"];
	auto texSRV = IBLEnvironmentMap->GetSRV();
	auto gfxCmdList = g_CommandContext.GetCommandList();

	g_CommandContext.Transition(IBLIrradianceMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	//g_CommandContext.Transition(IBLIrradianceMap->GetD3D12DepthBuffer(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	gfxCmdList->RSSetViewports(1, IBLIrradianceMap->GetViewport());
	gfxCmdList->RSSetScissorRects(1, IBLIrradianceMap->GetScissorRect());

	// set viewport and scirssor
	// set RootSignature and PSO
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	ObjCBuffer obj;
	PassCBuffer passcb;

	for (UINT i = 0; i < 6; i++)
	{
		float clearColor[4] = { 0.0,0.0,0.0,0.0 };
		gfxCmdList->ClearRenderTargetView(IBLIrradianceMap->GetRTV(i), (FLOAT*)clearColor, 0, nullptr);
		//gfxCmdList->ClearDepthStencilView(IBLIrradianceMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
		gfxCmdList->OMSetRenderTargets(1, &IBLIrradianceMap->GetRTV(i), TRUE, nullptr);

		// update CBuffer
		{
			auto camera = IBLIrradianceMap->GetCamera(i);
			XMStoreFloat4x4(&passcb.ViewMat, XMMatrixTranspose(camera.GetViewMat()));
			XMStoreFloat4x4(&passcb.ProjMat, XMMatrixTranspose(camera.GetProjMat()));
		}
		auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
		auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));


		shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
		shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
		shader.SetParameter("EnvironmentMap", texSRV);
		shader.SetDescriptorCache(boxMesh.GetTD3D12DescriptorCache());
		shader.BindParameters();
		boxMesh.DrawMesh(g_CommandContext);
	}

	// transition buffer state
	g_CommandContext.Transition(IBLIrradianceMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	//g_CommandContext.Transition(IBLIrradianceMap->GetD3D12DepthBuffer(), D3D12_RESOURCE_STATE_COMMON);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::CreateIBLPrefilterMap()
{
	g_CommandContext.ResetCommandList();

	for (UINT Mip = 0; Mip < IBLPrefilterMaxMipLevel; Mip++)
	{
		auto shader = PSOManager::m_shaderMap["prefilterMapShader"];
		auto pso = PSOManager::m_gfxPSOMap["prefilterMapPSO"];
		auto boxMesh = ModelManager::m_MeshMaps["box"];
		auto texSRV = IBLEnvironmentMap->GetSRV();
		auto gfxCmdList = g_CommandContext.GetCommandList();

		g_CommandContext.Transition(IBLPrefilterMaps[Mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

		gfxCmdList->RSSetViewports(1, IBLPrefilterMaps[Mip]->GetViewport());
		gfxCmdList->RSSetScissorRects(1, IBLPrefilterMaps[Mip]->GetScissorRect());

		// set viewport and scirssor
		// set RootSignature and PSO
		gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
		gfxCmdList->SetPipelineState(pso.GetPSO());

		ObjCBuffer obj;
		PassCBuffer passcb;
		passcb.Pad0 = (float)Mip / (float)(IBLPrefilterMaxMipLevel - 1);

		for (UINT i = 0; i < 6; i++)
		{
			float clearColor[4] = { 0.0,0.0,0.0,0.0 };
			gfxCmdList->ClearRenderTargetView(IBLPrefilterMaps[Mip]->GetRTV(i), (FLOAT*)clearColor, 0, nullptr);
			//gfxCmdList->ClearDepthStencilView(IBLIrradianceMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
			gfxCmdList->OMSetRenderTargets(1, &IBLPrefilterMaps[Mip]->GetRTV(i), TRUE, nullptr);

			// update CBuffer
			{
				auto camera = IBLPrefilterMaps[Mip]->GetCamera(i);
				XMStoreFloat4x4(&passcb.ViewMat, XMMatrixTranspose(camera.GetViewMat()));
				XMStoreFloat4x4(&passcb.ProjMat, XMMatrixTranspose(camera.GetProjMat()));
				// set roughness
			}

			auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
			auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

			shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
			shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
			shader.SetParameter("EnvironmentMap", texSRV);
			shader.SetDescriptorCache(boxMesh.GetTD3D12DescriptorCache());
			shader.BindParameters();
			boxMesh.DrawMesh(g_CommandContext);
		}

		// transition buffer state
		g_CommandContext.Transition(IBLPrefilterMaps[Mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	}
	g_CommandContext.ExecuteCommandLists();
}

void TRender::CreateIBLLUT2D()
{
	g_CommandContext.ResetCommandList();

	auto shader = PSOManager::m_shaderMap["brdfShader"];
	auto pso = PSOManager::m_gfxPSOMap["brdfPSO"];
	auto fullQuadMesh = ModelManager::m_MeshMaps["FullQuad"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	g_CommandContext.Transition(IBLBrdfLUT2D->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	gfxCmdList->RSSetViewports(1, IBLBrdfLUT2D->GetViewport());
	gfxCmdList->RSSetScissorRects(1, IBLBrdfLUT2D->GetScissorRect());

	// set viewport and scirssor
	// set RootSignature and PSO
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	{
		float clearColor[4] = { 0.0,0.0,0.0,0.0 };
		gfxCmdList->ClearRenderTargetView(IBLBrdfLUT2D->GetRTV(), (FLOAT*)clearColor, 0, nullptr);
		//gfxCmdList->ClearDepthStencilView(IBLIrradianceMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
		gfxCmdList->OMSetRenderTargets(1, &IBLBrdfLUT2D->GetRTV(), TRUE, nullptr);

		shader.SetDescriptorCache(fullQuadMesh.GetTD3D12DescriptorCache());
		shader.BindParameters();
		fullQuadMesh.DrawMesh(g_CommandContext);
	}

	// transition buffer state
	g_CommandContext.Transition(IBLIrradianceMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::CreateSceneCaptureCube()
{
	IBLEnvironmentMap = std::make_unique<SceneCaptureCube>(L"IBL Environment Map", 256, 256, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	IBLEnvironmentMap->CreateCubeCamera({ 0, 0, 0 }, 0.1, 10);

	IBLIrradianceMap = std::make_unique<SceneCaptureCube>(L"IBL Irradiance Map", 128, 128, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	IBLIrradianceMap->CreateCubeCamera({ 0, 0, 0 }, 0.1, 10);

	for (UINT Mip = 0; Mip < IBLPrefilterMaxMipLevel; Mip++)
	{
		UINT MipWidth = UINT(128 * std::pow(0.5, Mip));
		auto PrefilterMap = std::make_unique<SceneCaptureCube>(L"IBL Prefilter Map", MipWidth, MipWidth, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
		PrefilterMap->CreateCubeCamera({ 0, 0, 0 }, 0.1, 10);

		IBLPrefilterMaps.push_back(std::move(PrefilterMap));
	}

	IBLBrdfLUT2D = std::make_unique<RenderTarget2D>(L"IBL BRDF LUT2D", 128, 128, 1, DXGI_FORMAT_R8G8_UNORM);
}
