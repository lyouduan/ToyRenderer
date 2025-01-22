#include "Render.h"
#include "D3D12RHI.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Display.h"
#include "ImGuiManager.h"
#include "Light.h"

using namespace TD3D12RHI;
using namespace PSOManager;
TRender::TRender()
{
}

TRender::~TRender()
{
}

void TRender::Initalize()
{
	CreateSceneCaptureCube();

	CreateGBuffers();

	CreateForwardFulsBuffer();

}

void TRender::DrawMesh(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader, TD3D12ConstantBufferRef& passRef)
{
	auto obj = model.GetObjCBuffer();
	auto objCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

	auto meshes = model.GetMeshes();
	for (UINT i = 0; i < meshes.size(); ++i)
	{
		shader.SetParameter("objCBuffer", objCBufferRef);
		shader.SetParameter("passCBuffer", passRef);
		// draw call
		auto SRV = meshes[i].GetSRV();
		if (!SRV.empty())
		{
			shader.SetParameter("diffuseMap", SRV[0]);
			if (SRV.size() == 2)
			{
				shader.SetParameter("metallicMap", SRV[1]);
			}
			else
			{
				// binding NullDescriptor
				shader.SetParameter("metallicMap", NullDescriptor);
			}
		}
		else
		{
			shader.SetParameter("diffuseMap", NullDescriptor);
			shader.SetParameter("metallicMap", NullDescriptor);
		}

		shader.SetDescriptorCache(meshes[i].GetTD3D12DescriptorCache());
		shader.BindParameters(); // after binding Parameter, it will clear all Parameter
		meshes[i].DrawMesh(gfxContext);
	}
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
		float clearColor[4] = { 0.0, 0.0, 0.0, 0.0 };
		gfxCmdList->ClearRenderTargetView(IBLEnvironmentMap->GetRTV(i), (FLOAT*)clearColor, 0, nullptr);
		//gfxCmdList->ClearDepthStencilView(IBLIrradianceMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH, 1.0, 0, 0, nullptr);
		gfxCmdList->OMSetRenderTargets(1, &IBLEnvironmentMap->GetRTV(i), TRUE, nullptr);

		// update CBuffer
		{
			auto camera = IBLEnvironmentMap->GetCamera(i);
			XMStoreFloat4x4(&passcb.ViewMat, XMMatrixTranspose(camera.GetViewMat()));
			XMStoreFloat4x4(&passcb.ProjMat, XMMatrixTranspose(camera.GetProjMat()));
		}
		auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
		auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

		//shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
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
	auto texSRV = bUseEquirectangularMap ? IBLEnvironmentMap->GetSRV() : TextureManager::m_SrvMaps["skybox"];
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
	auto shader = PSOManager::m_shaderMap["prefilterMapShader"];
	auto pso = PSOManager::m_gfxPSOMap["prefilterMapPSO"];
	auto boxMesh = ModelManager::m_MeshMaps["box"];
	auto texSRV = bUseEquirectangularMap ? IBLEnvironmentMap->GetSRV() : TextureManager::m_SrvMaps["skybox"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	for (UINT Mip = 0; Mip < IBLPrefilterMaxMipLevel; Mip++)
	{
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

void TRender::GbuffersPass()
{
	auto gfxCmdList = g_CommandContext.GetCommandList();

	gfxCmdList->RSSetViewports(1, GBufferAlbedo->GetViewport());
	gfxCmdList->RSSetScissorRects(1, GBufferAlbedo->GetScissorRect());

	g_CommandContext.Transition(GBufferAlbedo->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(GBufferNormal->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(GBufferSpecular->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(GBufferWorldPos->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	//g_CommandContext.Transition(GBufferDepth->GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// clear renderTargets
	float clearColor[4] = { GBufferAlbedo->GetClearColor().x,GBufferAlbedo->GetClearColor().y,GBufferAlbedo->GetClearColor().z,GBufferAlbedo->GetClearColor().w };
	gfxCmdList->ClearRenderTargetView(GBufferAlbedo->GetRTV(), (float*)&clearColor, 0, nullptr);
	gfxCmdList->ClearRenderTargetView(GBufferNormal->GetRTV(), (float*)&clearColor, 0, nullptr);
	gfxCmdList->ClearRenderTargetView(GBufferSpecular->GetRTV(),  (float*)&clearColor, 0, nullptr);
	gfxCmdList->ClearRenderTargetView(GBufferWorldPos->GetRTV(),  (float*)&clearColor, 0, nullptr);
	//gfxCmdList->ClearRenderTargetView(GBufferDepth->GetRTV(),  (float*)&clearColor, 0, nullptr);

	gfxCmdList->ClearDepthStencilView(g_DepthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// copy CPU descriptor to GPU
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RtvDescriptors;
	RtvDescriptors.push_back(GBufferAlbedo->GetRTV());
	RtvDescriptors.push_back(GBufferSpecular->GetRTV());
	RtvDescriptors.push_back(GBufferWorldPos->GetRTV());
	RtvDescriptors.push_back(GBufferNormal->GetRTV());
	//RtvDescriptors.push_back(GBufferDepth->GetRTV());

	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	GBufferDescriptorCache->AppendRtvDescriptors(RtvDescriptors, GpuHandle, CpuHandle);

	gfxCmdList->OMSetRenderTargets(RtvDescriptors.size(), &CpuHandle, true, &g_DepthBuffer.GetDSV());
	
	gfxCmdList->SetGraphicsRootSignature(m_gfxPSOMap["GBuffersPso"].GetRootSignature());
	gfxCmdList->SetPipelineState(m_gfxPSOMap["GBuffersPso"].GetPSO());


	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(g_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(g_Camera.GetProjMat()));
	passCB.EyePosition = g_Camera.GetPosition3f();
	passCB.lightPos = ImGuiManager::lightPos;
	auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));

	// draw all mesh
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-10 + i*10, 0, j*10);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["nanosuit"].SetObjCBuffer(obj);

			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["nanosuit"], m_shaderMap["GBuffersPassShader"], passCBufferRef);
		}
	}

	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["wall"], m_shaderMap["GBuffersPassShader"], passCBufferRef);
	
	// transit to generic read state
	g_CommandContext.Transition(GBufferAlbedo->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferNormal->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferSpecular->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferWorldPos->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	//g_CommandContext.Transition(GBufferDepth->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	GBufferDescriptorCache->Reset();

	// Debug
}

void TRender::DeferredShadingPass()
{
	auto pso = PSOManager::m_gfxPSOMap["DeferredShadingPso"];
	auto shader = m_shaderMap["DeferredShadingShader"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(g_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(g_Camera.GetProjMat()));
	passCB.EyePosition = g_Camera.GetPosition3f();
	passCB.lightPos = ImGuiManager::lightPos;
	passCB.lightColor = ImGuiManager::lightColor;
	passCB.Intensity = ImGuiManager::Intensity;
	auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));

	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	shader.SetParameter("passCBuffer", passCBufferRef);
	shader.SetParameter("GBufferAlbedoMap", GBufferAlbedo->GetSRV());
	shader.SetParameter("GBufferWorldPosMap", GBufferWorldPos->GetSRV());
	shader.SetParameter("GBufferNormalMap", GBufferNormal->GetSRV());
	shader.SetParameter("GBufferSpecularMap", GBufferSpecular->GetSRV());

	shader.SetDescriptorCache(ModelManager::m_MeshMaps["FullQuad"].GetTD3D12DescriptorCache());
	shader.BindParameters();
	ModelManager::m_MeshMaps["FullQuad"].DrawMesh(g_CommandContext);

}

void TRender::GbuffersDebugPass()
{
	auto pso = PSOManager::m_gfxPSOMap["DebugQuadPSO"];
	auto shader = m_shaderMap["DebugQuadShader"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	D3D12_CPU_DESCRIPTOR_HANDLE texSRV;
	switch ((GBufferType)ImGuiManager::GbufferType)
	{
	case GBufferType::Position:
		texSRV = GBufferWorldPos->GetSRV();
		break;
	case GBufferType::Normal:
		texSRV = GBufferNormal->GetSRV();
		break;
	case GBufferType::Albedo:
		texSRV = GBufferAlbedo->GetSRV();
		break;
	case GBufferType::Specular:
		texSRV = GBufferSpecular->GetSRV();
		break;
	//case GBufferType::Depth:
	//	texSRV = GBufferDepth->GetSRV();
	//	break;
	default:
		texSRV = NullDescriptor;
		break;
	}

	auto srv = DebugMap->GetSRV();
	shader.SetParameter("tex", srv);
	
	shader.SetDescriptorCache(ModelManager::m_MeshMaps["DebugQuad"].GetTD3D12DescriptorCache());
	shader.BindParameters();
	ModelManager::m_MeshMaps["DebugQuad"].DrawMesh(g_CommandContext);
}

void TRender::PrePassDepthBuffer()
{
	auto shader = PSOManager::m_shaderMap["preDepthPass"];
	auto pso = PSOManager::m_gfxPSOMap["preDepthPassPSO"];
	auto gfxCmdList = g_CommandContext.GetCommandList();
	
	// set viewport and scissorRect
	D3D12_VIEWPORT m_Viewport = { 0.0, 0.0, (float)g_DisplayWidth, (float)g_DisplayHeight, 0.0, 1.0 };
	D3D12_RECT m_ScissorRect = { 0, 0, (int)m_Viewport.Width, (int)m_Viewport.Height };
	gfxCmdList->RSSetViewports(1, &m_Viewport);
	gfxCmdList->RSSetScissorRects(1, &m_ScissorRect);

	// set PSO and RootSignature
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	g_CommandContext.Transition(g_PreDepthPassBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	gfxCmdList->ClearDepthStencilView(g_PreDepthPassBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);
	gfxCmdList->OMSetRenderTargets(0, nullptr, false, &g_PreDepthPassBuffer.GetDSV());

	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(g_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(g_Camera.GetProjMat()));
	passCB.EyePosition = g_Camera.GetPosition3f();
	passCB.lightPos = ImGuiManager::lightPos;
	auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));

	// draw all mesh
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-10 + i * 10, 0, j * 10);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["nanosuit"].SetObjCBuffer(obj);
	
			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["nanosuit"], shader, passCBufferRef);
		}
	}
	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["wall"], shader, passCBufferRef);

	// transit to generic read state
	g_CommandContext.Transition(g_PreDepthPassBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void TRender::CullingLightPass()
{
	// Cull light

	auto pso = PSOManager::m_ComputePSOMap["CullLight"];
	auto shader = PSOManager::m_shaderMap["CullLight"];

	auto computeCmdList = g_CommandContext.GetCommandList();
	computeCmdList->SetComputeRootSignature(pso.GetRootSignature());
	computeCmdList->SetPipelineState(pso.GetPSO());

	auto DepthSRV = g_PreDepthPassBuffer.GetSRV();
	shader.SetParameter("ScreenToViewParams", ConstantBufferRef);
	shader.SetParameter("DispatchParams", DispatchParamsCBRef);
	shader.SetParameter("DepthTextureVS", DepthSRV);
	shader.SetParameter("Lights", LightManager::g_light.GetStructuredBuffer()->GetSRV());
	shader.SetParameter("in_Frustums", RWStructuredBufferRef->GetSRV());

	shader.SetParameterUAV("o_LightIndexCounter", LightIndexCounterCBRef->GetUAV());
	shader.SetParameterUAV("o_LightIndexList", LightIndexListCBRef->GetUAV());
	shader.SetParameterUAV("o_LightGrid", LightGridMap->GetUAV());

	shader.SetParameterUAV("DebugTexture",DebugMap->GetUAV());

	shader.BindParameters();

	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);
	computeCmdList->Dispatch(numGroupsX, numGroupsY, 1);
}

void TRender::LightPass()
{
	Light light = LightManager::g_light;
	// light 

	auto lights = light.GetLightInfo();

	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["lightPSO"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["lightPSO"].GetPSO());

	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(g_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(g_Camera.GetProjMat()));
	passCB.EyePosition = g_Camera.GetPosition3f();
	passCB.lightPos = ImGuiManager::lightPos;

	ObjCBuffer objCB;

	for(int i = 0; i < lights.size(); i++)
	{
		objCB.ModelMat = lights[i].ModelMat;
		passCB.Pad0 = i;

		auto ObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&objCB, sizeof(ObjCBuffer));
		auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));
		
		m_shaderMap["lightShader"].SetParameter("objCBuffer", ObjCBufferRef);
		m_shaderMap["lightShader"].SetParameter("passCBuffer", passCBufferRef);
		m_shaderMap["lightShader"].SetParameter("Lights", light.GetStructuredBuffer()->GetSRV());
		m_shaderMap["lightShader"].SetDescriptorCache(ModelManager::m_MeshMaps["sphere"].GetTD3D12DescriptorCache());
		m_shaderMap["lightShader"].BindParameters();
		ModelManager::m_MeshMaps["sphere"].DrawMesh(g_CommandContext);
	}
}

void TRender::BuildTileFrustums()
{
	g_CommandContext.ResetCommandList();

	auto computeCmdList = g_CommandContext.GetCommandList();
	auto pso = PSOManager::m_ComputePSOMap["ForwardPuls"];
	auto shader = PSOManager::m_shaderMap["ForwardPuls"];

	computeCmdList->SetComputeRootSignature(pso.GetRootSignature());
	computeCmdList->SetPipelineState(pso.GetPSO());

	// Shader binding
	shader.SetParameter("ScreenToViewParams", ConstantBufferRef);
	shader.SetParameter("DispatchParams", DispatchParamsCBRef);
	shader.SetParameterUAV("out_Frustums", RWStructuredBufferRef->GetUAV());
	shader.BindParameters();

	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);
	computeCmdList->Dispatch(numGroupsX, numGroupsY, 1);

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

	IBLBrdfLUT2D = std::make_unique<RenderTarget2D>(L"IBL BRDF LUT2D", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R8G8_UNORM);
}

void TRender::CreateGBuffers()
{
	GBufferAlbedo = std::make_unique<RenderTarget2D>(L"Gbuffer Albedo", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GBufferSpecular = std::make_unique<RenderTarget2D>(L"Gbuffer Specular", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GBufferWorldPos = std::make_unique<RenderTarget2D>(L"Gbuffer WorldPosition", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GBufferNormal = std::make_unique<RenderTarget2D>(L"Gbuffer Normal", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	//GBufferDepth = std::make_unique<RenderTarget2D>(L"Gbuffer Depth", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

	GBufferDescriptorCache = std::make_unique<TD3D12DescriptorCache>(TD3D12RHI::g_Device);


	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	GraphicsPSO GBuffersPso(L"GBuffers PSO");
	GBuffersPso.SetShader(&m_shaderMap["GBuffersPassShader"]);
	GBuffersPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
	GBuffersPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
	GBuffersPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	CD3DX12_DEPTH_STENCIL_DESC dsvDesc;
	dsvDesc.DepthEnable = TRUE;
	dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	dsvDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	dsvDesc.StencilEnable = FALSE;
	GBuffersPso.SetDepthStencilState(dsvDesc);
	GBuffersPso.SetSampleMask(UINT_MAX);
	GBuffersPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
	DXGI_FORMAT GBuffersFormat[] =
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		//DXGI_FORMAT_R16G16B16A16_FLOAT
	};
	GBuffersPso.SetRenderTargetFormats(_countof(GBuffersFormat), GBuffersFormat, g_DepthBuffer.GetFormat());
	GBuffersPso.Finalize();
	m_gfxPSOMap["GBuffersPso"] = GBuffersPso;


	GraphicsPSO DeferredShadingPso(L"DeferredShading PSO");
	DeferredShadingPso.SetShader(&m_shaderMap["DeferredShadingShader"]);
	DeferredShadingPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
	DeferredShadingPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
	DeferredShadingPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
	DeferredShadingPso.SetDepthStencilState(dsvDesc);
	DeferredShadingPso.SetSampleMask(UINT_MAX);
	DeferredShadingPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	DeferredShadingPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
	DeferredShadingPso.Finalize();
	m_gfxPSOMap["DeferredShadingPso"] = DeferredShadingPso;
}

void TRender::CreateForwardFulsBuffer()
{
	struct Plane
	{
		XMFLOAT3 N; // plane normal
		float  d; // distance to origin
	};
	struct Frustum
	{
		// left, right, top, bottom frustum planes.
		Plane Planes[4];
	};

	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);

	RWStructuredBufferRef = TD3D12RHI::CreateRWStructuredBuffer(sizeof(Frustum),numGroupsX * numGroupsY);

	struct ScreenToViewParams
	{
		XMFLOAT4X4 InverseProjection;
		XMFLOAT2 ScreenDimensions;
	};

	ScreenToViewParams stv;
	XMVECTOR det = XMMatrixDeterminant(g_Camera.GetProjMat());
	XMMATRIX invProj = XMMatrixInverse(&det, g_Camera.GetProjMat());
	XMStoreFloat4x4(&stv.InverseProjection, invProj);

	stv.ScreenDimensions = { (float)g_DisplayWidth, (float)g_DisplayHeight };

	ConstantBufferRef = TD3D12RHI::CreateConstantBuffer(&stv, sizeof(ScreenToViewParams));


	struct DispatchParams
	{
		// Number of groups dispatched.
		UINT numThreadGroups[3];
		UINT padding0;
	// uint padding //implicit padding to 16 bytes 

	// Total number of threads dispatched.
	// Note: This value may be less than the actual number of threads executed
	// if the screen size is not evenly divisible by the block size
		UINT numThreads[3];
		UINT padding1;
	// uint padding //implicit padding to 16 bytes 
	};

	DispatchParams dp = { {numGroupsX, numGroupsY, 1}, 0,  {g_DisplayWidth, g_DisplayHeight} ,0};

	DispatchParamsCBRef = TD3D12RHI::CreateConstantBuffer(&dp, sizeof(DispatchParams));

	LightIndexCounterCBRef = TD3D12RHI::CreateRWStructuredBuffer(sizeof(UINT), 1);

	LightIndexListCBRef = TD3D12RHI::CreateRWStructuredBuffer(sizeof(UINT), 512 * numGroupsX * numGroupsY);

	LightGridMap = std::make_unique<D3D12ColorBuffer>() ;
	LightGridMap->Create(L"Light Grip Map", numGroupsX, numGroupsY, 1, DXGI_FORMAT_R32G32_UINT);

	DebugMap = std::make_unique<D3D12ColorBuffer>();
	DebugMap->Create(L"Debug Map", numGroupsX, numGroupsY, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
}
