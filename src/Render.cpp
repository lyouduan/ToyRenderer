#include "Render.h"
#include "D3D12RHI.h"
#include "PSOManager.h"
#include "TextureManager.h"
#include "ModelManager.h"
#include "Display.h"
#include "ImGuiManager.h"
#include "Light.h"
#include "RenderInfo.h"

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

	CreateGBuffersResource();
	CreateGBuffersPSO(); // need initializing GBuffers first

	CreateForwardPulsResource();

	CreateShadowResource();

	CreateCSMResource();
}

void TRender::SetDescriptorHeaps()
{
	// set descriptor heaps
	ID3D12DescriptorHeap* ppHeaps[] = { TD3D12RHI::g_DescriptorCache->GetCacheCbvSrvUavDescriptorHeap().Get()};
	g_CommandContext.GetCommandList()->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
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

		

		if (bEnableForwardPuls)
		{
			shader.SetParameter("LightInfoList", TileLightInfoListRef->GetSRV());
			shader.SetParameter("Lights", LightManager::g_light.GetStructuredBuffer()->GetSRV());
		}

		if (bEnableShadowMap)
		{
			shader.SetParameter("ShadowMap", m_ShadowMap->GetSRV());
			shader.SetParameter("VSMTexture", m_VSMTexture->GetSRV());
			shader.SetParameter("ESMTexture", m_ESMTexture->GetSRV());
			shader.SetParameter("EVSMTexture", m_EVSMTexture->GetSRV());

			// VSSM
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SRVHandleList;
			for (UINT i = 0; i < m_VSSMTextures.size(); ++i)
			{
				SRVHandleList.push_back(m_VSSMTextures[i]->GetSRV());
			}
			shader.SetParameter("VSSMTextures", SRVHandleList);
			
			// CSM
			SRVHandleList.clear();
			LightInfo info = LightManager::DirectionalLight.GetLightInfo()[0];
			for (UINT i = 0; i < m_CascadedShadowMap->GetCascadeCount(); ++i)
			{
				SRVHandleList.push_back(m_CascadedShadowMap->GetSRV(i));
				XMStoreFloat4x4(&info.CSMTransform[i], XMMatrixTranspose(m_CascadedShadowMap->GetShadowTransform(i)));
			}
			shader.SetParameter("CSMTextures", SRVHandleList);

			CSMCBuffer csm;
			auto frustumVSFarZ = m_CascadedShadowMap->GetFrustumVSFarZ();
			for (int i = 0; i < frustumVSFarZ.size(); ++i)
			{
				csm.frustumVSFarZ[i] = XMFLOAT2(frustumVSFarZ[i].x, frustumVSFarZ[i].y);
			}
			auto CSMCBRef = TD3D12RHI::CreateConstantBuffer(&csm, sizeof(CSMCBuffer));
			shader.SetParameter("CSMCBuffer", CSMCBRef);

			LightManager::DirectionalLight.SetLightInfo(info);
			LightManager::DirectionalLight.CreateStructuredBufferRef();
			shader.SetParameter("Lights", LightManager::DirectionalLight.GetStructuredBuffer()->GetSRV());
			
		}

		shader.BindParameters(); // after binding Parameter, it will clear all Parameter
		meshes[i].DrawMesh(gfxContext);
	}
}

void TRender::DrawMeshIBL(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader, TD3D12ConstantBufferRef& passRef)
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

		if (bEnableIBLEnvLighting)
		{
			// pbr Maps
			shader.SetParameter("diffuseMap", TextureManager::m_SrvMaps["Cerberus_A"]);
			shader.SetParameter("metallicMap", TextureManager::m_SrvMaps["Cerberus_M"]);
			shader.SetParameter("normalMap", TextureManager::m_SrvMaps["Cerberus_N"]);
			shader.SetParameter("roughnessMap", TextureManager::m_SrvMaps["Cerberus_R"]);

			// IBL Maps
			shader.SetParameter("IrradianceMap", IBLIrradianceMap->GetSRV());
			std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SRVHandleList;
			for (UINT i = 0; i < IBLPrefilterMaps.size(); ++i)
			{
				SRVHandleList.push_back(IBLPrefilterMaps[i]->GetSRV());
			}
			shader.SetParameter("PrefilterMap", SRVHandleList);
			shader.SetParameter("BrdfLUT2D", IBLBrdfLUT2D->GetSRV());
		}


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
			XMStoreFloat4x4(&passcb.invProjMat, XMMatrixTranspose(camera.GetInvProjMat()));

		}
		auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
		auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

		//shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
		shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
		shader.SetParameter("equirectangularMap", texSRV);
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
			XMStoreFloat4x4(&passcb.invProjMat, XMMatrixTranspose(camera.GetInvProjMat()));
		}
		auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
		auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));


		shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
		shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
		shader.SetParameter("EnvironmentMap", texSRV);
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
				XMStoreFloat4x4(&passcb.invProjMat, XMMatrixTranspose(camera.GetInvProjMat()));
			}

			auto cubemapPassCBufferRef = TD3D12RHI::CreateConstantBuffer(&passcb, sizeof(PassCBuffer));
			auto cubemapObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

			shader.SetParameter("objCBuffer", cubemapObjCBufferRef); // don't need objCbuffer
			shader.SetParameter("passCBuffer", cubemapPassCBufferRef);
			shader.SetParameter("EnvironmentMap", texSRV);
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

		shader.BindParameters();
		fullQuadMesh.DrawMesh(g_CommandContext);
	}

	// transition buffer state
	g_CommandContext.Transition(IBLIrradianceMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::IBLRenderPass()
{
	// Record commands
	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["pso"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["pso"].GetPSO());

	// update passCBuffer
	auto passCBufferRef = UpdatePassCbuffer();

	// mat CBuffer
	auto matCBufferRef = TD3D12RHI::CreateConstantBuffer(&ImGuiManager::matCB, sizeof(MatCBuffer));

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			ObjCBuffer obj;
			const XMVECTOR rotationAxisX = XMVectorSet(1, 0, 0, 0);
			XMMATRIX rotationMat = XMMatrixRotationAxis(rotationAxisX, XMConvertToRadians(90));
			auto scalingMat = XMMatrixScaling(0.05, 0.05, 0.05);
			auto rotationAxisY = XMVectorSet(0, 1, 0, 0);
			auto rotationMatY = rotationMat * XMMatrixRotationAxis(rotationAxisY, XMConvertToRadians(-45));

			XMMATRIX translationMatrix = XMMatrixTranslation(-10 + i * 10, 0, j * 10);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(rotationMatY * scalingMat * translationMatrix));
			ModelManager::m_ModelMaps["Cerberus_LP"].SetObjCBuffer(obj);

			DrawMeshIBL(g_CommandContext, ModelManager::m_ModelMaps["Cerberus_LP"], m_shaderMap["modelShader"], passCBufferRef);
		}
	}

	// pbr sphere 
	{
		XMMATRIX scalingMat = XMMatrixScaling(ImGuiManager::scale * 0.5, ImGuiManager::scale * 0.5, ImGuiManager::scale * 0.5);
		float rotate_angle = static_cast<float>(ImGuiManager::RotationY * 360);
		const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
		XMMATRIX rotationYMat = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(rotate_angle));
		XMMATRIX translationMatrix = XMMatrixTranslation(ImGuiManager::modelPosition.x, ImGuiManager::modelPosition.y, ImGuiManager::modelPosition.z);
		auto ModelMatrix = scalingMat * rotationYMat * translationMatrix;

		ObjCBuffer obj;
		XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(ModelMatrix));
		auto objCBufferRef = CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

		g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["pbrPSO"].GetRootSignature());
		g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["pbrPSO"].GetPSO());

		m_shaderMap["pbrShader"].SetParameter("objCBuffer", objCBufferRef);
		m_shaderMap["pbrShader"].SetParameter("matCBuffer", matCBufferRef);
		m_shaderMap["pbrShader"].SetParameter("passCBuffer", passCBufferRef);

		// IBL SRV
		m_shaderMap["pbrShader"].SetParameter("IrradianceMap", IBLIrradianceMap->GetSRV());
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SRVHandleList;
		for (UINT i = 0; i < IBLPrefilterMaps.size(); ++i)
		{
			SRVHandleList.push_back(IBLPrefilterMaps[i]->GetSRV());
		}
		m_shaderMap["pbrShader"].SetParameter("PrefilterMap", SRVHandleList);
		m_shaderMap["pbrShader"].SetParameter("BrdfLUT2D", IBLBrdfLUT2D->GetSRV());

		m_shaderMap["pbrShader"].BindParameters();
		ModelManager::m_MeshMaps["sphere"].DrawMesh(g_CommandContext);
	}

	// sky box
	{
		g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["skyboxPSO"].GetRootSignature());
		g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["skyboxPSO"].GetPSO());

		m_shaderMap["skyboxShader"].SetParameter("passCBuffer", passCBufferRef);

		if (bUseEquirectangularMap)
			m_shaderMap["skyboxShader"].SetParameter("CubeMap", IBLEnvironmentMap->GetSRV());
		else
			m_shaderMap["skyboxShader"].SetParameter("CubeMap", TextureManager::m_SrvMaps["skybox"]);

		//m_shaderMap["skyboxShader"].SetParameter("CubeMap", m_Render->GetIBLIrradianceMap()->GetSRV());
		m_shaderMap["skyboxShader"].BindParameters();
		ModelManager::m_MeshMaps["box"].DrawMesh(g_CommandContext);
	}
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
	g_CommandContext.Transition(GBufferDepth->GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// clear renderTargets
	float clearColor[4] = { GBufferAlbedo->GetClearColor().x,GBufferAlbedo->GetClearColor().y,GBufferAlbedo->GetClearColor().z,GBufferAlbedo->GetClearColor().w };
	gfxCmdList->ClearRenderTargetView(GBufferAlbedo->GetRTV(), (float*)&clearColor, 0, nullptr);
	gfxCmdList->ClearRenderTargetView(GBufferNormal->GetRTV(), (float*)&clearColor, 0, nullptr);
	gfxCmdList->ClearRenderTargetView(GBufferSpecular->GetRTV(),  (float*)&clearColor, 0, nullptr);
	gfxCmdList->ClearRenderTargetView(GBufferWorldPos->GetRTV(),  (float*)&clearColor, 0, nullptr);

	gfxCmdList->ClearDepthStencilView(GBufferDepth->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// copy CPU descriptor to GPU
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RtvDescriptors;
	RtvDescriptors.push_back(GBufferAlbedo->GetRTV());
	RtvDescriptors.push_back(GBufferSpecular->GetRTV());
	RtvDescriptors.push_back(GBufferWorldPos->GetRTV());
	RtvDescriptors.push_back(GBufferNormal->GetRTV());

	CD3DX12_CPU_DESCRIPTOR_HANDLE CpuHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE GpuHandle;
	g_DescriptorCache->AppendRtvDescriptors(RtvDescriptors, GpuHandle, CpuHandle);

	gfxCmdList->OMSetRenderTargets(RtvDescriptors.size(), &CpuHandle, true, &GBufferDepth->GetDSV());
	
	gfxCmdList->SetGraphicsRootSignature(m_gfxPSOMap["GBuffersPso"].GetRootSignature());
	gfxCmdList->SetPipelineState(m_gfxPSOMap["GBuffersPso"].GetPSO());


	auto passCBufferRef = UpdatePassCbuffer();

	// draw all mesh
	//for (int i = 0; i < 3; i++)
	//{
	//	for (int j = 0; j < 3; j++)
	//	{
	//		ObjCBuffer obj;
	//		XMMATRIX translationMatrix = XMMatrixTranslation(-10 + i*10, 0, j*10);
	//		XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
	//		ModelManager::m_ModelMaps["nanosuit"].SetObjCBuffer(obj);
	//
	//		DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["nanosuit"], m_shaderMap["GBuffersPassShader"], passCBufferRef);
	//	}
	//}
	//
	//DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["wall"], m_shaderMap["GBuffersPassShader"], passCBufferRef);
	
	// draw all mesh
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-42.5 + i * 20, 5, -42.5 + j * 20);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["Cylinder"].SetObjCBuffer(obj);
			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["Cylinder"], m_shaderMap["GBuffersPassShader"], passCBufferRef);
		}
	}
	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["floor"], m_shaderMap["GBuffersPassShader"], passCBufferRef);

	// transit to generic read state
	g_CommandContext.Transition(GBufferAlbedo->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferNormal->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferSpecular->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferWorldPos->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(GBufferDepth->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void TRender::TiledBaseLightCulling()
{
	// Cull light

	auto pso = PSOManager::m_ComputePSOMap["LightCulling"];
	auto shader = PSOManager::m_shaderMap["LightCulling"];

	auto computeCmdList = g_CommandContext.GetCommandList();
	computeCmdList->SetComputeRootSignature(pso.GetRootSignature());
	computeCmdList->SetPipelineState(pso.GetPSO());

	g_CommandContext.Transition(GBufferDepth->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(TileLightInfoListRef->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	g_CommandContext.Transition(TiledDepthDebugTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);


	shader.SetParameter("DepthTexture", GBufferDepth->GetSRV());
	shader.SetParameter("Lights", LightManager::g_light.GetStructuredBuffer()->GetSRV());
	shader.SetParameterUAV("TileLightInfoList", TileLightInfoListRef->GetUAV());
	shader.SetParameterUAV("TiledDepthDebugTexture", TiledDepthDebugTexture->GetUAV());

	auto passCBufferRef = UpdatePassCbuffer();
	shader.SetParameter("passCBuffer", passCBufferRef);
	shader.BindParameters();

	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);
	computeCmdList->Dispatch(numGroupsX, numGroupsY, 1);

	g_CommandContext.Transition(TileLightInfoListRef->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(TiledDepthDebugTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void TRender::DeferredShadingPass()
{
	auto pso = PSOManager::m_gfxPSOMap["DeferredShadingPso"];
	auto shader = m_shaderMap["DeferredShadingShader"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	auto passCBufferRef = UpdatePassCbuffer();

	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());
	
	shader.SetParameter("passCBuffer", passCBufferRef);
	shader.SetParameter("GBufferAlbedoMap", GBufferAlbedo->GetSRV());
	shader.SetParameter("GBufferWorldPosMap", GBufferWorldPos->GetSRV());
	shader.SetParameter("GBufferNormalMap", GBufferNormal->GetSRV());
	shader.SetParameter("GBufferSpecularMap", GBufferSpecular->GetSRV());
	
	shader.SetParameter("Lights", LightManager::g_light.GetStructuredBuffer()->GetSRV());
	shader.SetParameter("LightInfoList", TileLightInfoListRef->GetSRV());

	shader.BindParameters();
	ModelManager::m_MeshMaps["FullQuad"].DrawMesh(g_CommandContext);

}

void TRender::GbuffersDebug()
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
	case GBufferType::LightMap:
		texSRV = TiledDepthDebugTexture->GetSRV();
		break;
	default:
		texSRV = NullDescriptor;
		break;
	}

	shader.SetParameter("tex", texSRV);
	
	shader.BindParameters();
	ModelManager::m_MeshMaps["DebugQuad"].DrawMesh(g_CommandContext);
}

void TRender::SSAO()
{

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


	auto passCBufferRef = UpdatePassCbuffer();

	// draw all mesh
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-42.5 + i * 20, 5, -42.5 + j * 20);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["Cylinder"].SetObjCBuffer(obj);
			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["Cylinder"], shader, passCBufferRef);
		}
	}
	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["floor"], shader, passCBufferRef);

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


	g_CommandContext.Transition(TileLightInfoListRef->GetResource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	g_CommandContext.Transition(DebugMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	
	auto passCBufferRef = UpdatePassCbuffer();
	shader.SetParameter("passCBuffer", passCBufferRef);
	shader.SetParameter("DepthTexture", g_PreDepthPassBuffer.GetSRV());
	shader.SetParameter("Lights", LightManager::g_light.GetStructuredBuffer()->GetSRV());
	shader.SetParameterUAV("TileLightInfoList", TileLightInfoListRef->GetUAV());
	shader.SetParameterUAV("DebugTexture",DebugMap->GetUAV());
	shader.BindParameters();

	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);
	computeCmdList->Dispatch(numGroupsX, numGroupsY, 1);

	g_CommandContext.Transition(TileLightInfoListRef->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(DebugMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
}

void TRender::ForwardPlusPass()
{
	auto shader = PSOManager::m_shaderMap["ForwardPulsPass"];
	auto pso = PSOManager::m_gfxPSOMap["ForwardPulsPass"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	// set PSO and RootSignature
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	auto passCBufferRef = UpdatePassCbuffer();

	// draw all mesh
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-42.5 + i * 20, 5, -42.5 + j * 20);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["Cylinder"].SetObjCBuffer(obj);
			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["Cylinder"], shader, passCBufferRef);
		}
	}

	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["floor"], shader, passCBufferRef);
}

void TRender::LightGridDebug()
{
	{
		auto pso = PSOManager::m_gfxPSOMap["ShadowMapDebug"];
		auto shader = m_shaderMap["ShadowMapDebug"];
		auto gfxCmdList = g_CommandContext.GetCommandList();

		gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
		gfxCmdList->SetPipelineState(pso.GetPSO());

		auto texSRV = DebugMap->GetSRV();
		shader.SetParameter("tex", texSRV);

		shader.BindParameters();
		ModelManager::m_MeshMaps["DebugQuad"].DrawMesh(g_CommandContext);
	}
}

void TRender::LightPass()
{
	Light light = LightManager::g_light;
	// light 
	auto lights = light.GetLightInfo();

	Mesh pointLight = ModelManager::m_MeshMaps["SpotLight"];

	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["lightPSO"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["lightPSO"].GetPSO());


	auto passCBufferRef = UpdatePassCbuffer();

	ObjCBuffer objCB;

	struct LightIndex
	{
		UINT Index;
	};
	LightIndex lightIndex;

	for(int i = 0; i < lights.size(); i++)
	{
		objCB.ModelMat = lights[i].ModelMat;
		lightIndex.Index = i;

		auto ObjCBufferRef = TD3D12RHI::CreateConstantBuffer(&objCB, sizeof(ObjCBuffer));
		auto lightIndexRef = TD3D12RHI::CreateConstantBuffer(&lightIndex, sizeof(LightIndex));

		m_shaderMap["lightShader"].SetParameter("objCBuffer", ObjCBufferRef);
		m_shaderMap["lightShader"].SetParameter("passCBuffer", passCBufferRef);
		m_shaderMap["lightShader"].SetParameter("LightIndex", lightIndexRef);
		m_shaderMap["lightShader"].SetParameter("Lights", light.GetStructuredBuffer()->GetSRV());
		m_shaderMap["lightShader"].BindParameters();
		pointLight.DrawMesh(g_CommandContext);
	}
}

void TRender::ShadowPass()
{
	g_CommandContext.ResetCommandList();

	auto shader = PSOManager::m_shaderMap["preDepthPass"];
	auto pso = PSOManager::m_gfxPSOMap["preDepthPassPSO"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	// set viewport and scissorRect
	gfxCmdList->RSSetViewports(1, m_ShadowMap->GetViewport());
	gfxCmdList->RSSetScissorRects(1, m_ShadowMap->GetScissorRect());

	// transition buffer state
	g_CommandContext.Transition(m_ShadowMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
	// set render target
	gfxCmdList->ClearDepthStencilView(m_ShadowMap->GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);
	gfxCmdList->OMSetRenderTargets(0, nullptr, false, &m_ShadowMap->GetDSV());

	// set PSO and RootSignature
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	// UpdateShadowCB
	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(m_ShadowMap->GetLightView()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(m_ShadowMap->GetLightProj()));
	XMVECTOR det;
	XMMATRIX invProj = XMMatrixInverse(&det, m_ShadowMap->GetLightProj());
	XMStoreFloat4x4(&passCB.invProjMat, XMMatrixTranspose(invProj));

	auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));

	// draw all mesh
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-42.5 + i * 20, 5, -42.5 + j * 20);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["Cylinder"].SetObjCBuffer(obj);
			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["Cylinder"], shader, passCBufferRef);
		}
	}
	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["floor"], shader, passCBufferRef);

	g_CommandContext.Transition(m_ShadowMap->GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_READ);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::ShadowMapDebug()
{
	auto pso = PSOManager::m_gfxPSOMap["ShadowMapDebug"];
	auto shader = m_shaderMap["ShadowMapDebug"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());

	auto texSRV = m_ShadowMap->GetSRV();
	if(ImGuiManager::ShadowType == (int)ShadowType::CSM)
		texSRV = m_CascadedShadowMap->GetSRV(0);

	shader.SetParameter("tex", texSRV);

	shader.BindParameters();
	ModelManager::m_MeshMaps["DebugQuad"].DrawMesh(g_CommandContext);
}

void TRender::ScenePass()
{
	TShader shader;
	GraphicsPSO pso;
	switch ((ShadowType)ImGuiManager::ShadowType)
	{
	case ShadowType::NONE:
		shader = PSOManager::m_shaderMap["NoShadowMap"];
		pso = PSOManager::m_gfxPSOMap["NoShadowMap"];
		break;
	case ShadowType::PCSS:
		shader = PSOManager::m_shaderMap["ShadowMapPCSS"];
		pso = PSOManager::m_gfxPSOMap["ShadowMapPCSS"];
		break;
	case ShadowType::VSM:
		shader = PSOManager::m_shaderMap["ShadowMapVSM"];
		pso = PSOManager::m_gfxPSOMap["ShadowMapVSM"];
		break;
	case ShadowType::VSSM:
		shader = PSOManager::m_shaderMap["ShadowMapVSSM"];
		pso = PSOManager::m_gfxPSOMap["ShadowMapVSSM"];
		break;
	case ShadowType::ESM:
		shader = PSOManager::m_shaderMap["ShadowMapESM"];
		pso = PSOManager::m_gfxPSOMap["ShadowMapESM"];
		break;
	case ShadowType::EVSM:
		shader = PSOManager::m_shaderMap["ShadowMapEVSM"];
		pso = PSOManager::m_gfxPSOMap["ShadowMapEVSM"];
		break;
	case ShadowType::CSM:
		shader = PSOManager::m_shaderMap["CSM"];
		pso = PSOManager::m_gfxPSOMap["CSM"];
		break;
	default: // PCF
		shader = PSOManager::m_shaderMap["ShadowMap"];
		pso = PSOManager::m_gfxPSOMap["ShadowMap"];
		break;
	}

	auto gfxCmdList = g_CommandContext.GetCommandList();

	// set PSO and RootSignature
	gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
	gfxCmdList->SetPipelineState(pso.GetPSO());


	auto passCBufferRef = UpdatePassCbuffer();

	// draw all mesh
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			ObjCBuffer obj;
			XMMATRIX translationMatrix = XMMatrixTranslation(-42.5 + i * 20, 5, -42.5 + j * 20);
			XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
			ModelManager::m_ModelMaps["Cylinder"].SetObjCBuffer(obj);
			DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["Cylinder"], shader, passCBufferRef);
		}
	}

	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["floor"], shader, passCBufferRef);
}

void TRender::GenerateSAT()
{
	// --------------- Get Summed Area Table ---------------
	g_CommandContext.ResetCommandList();

	auto computeCmdList = g_CommandContext.GetCommandList();

	// Set PSO and RootSignature
	auto pso = PSOManager::m_ComputePSOMap["localCS"];
	auto shader = PSOManager::m_shaderMap["localCS"];
	// Set Shader Parameter
	computeCmdList->SetComputeRootSignature(pso.GetRootSignature());
	computeCmdList->SetPipelineState(pso.GetPSO());

	shader.SetParameter("InputTexture", m_ShadowMap->GetSRV());
	shader.SetParameterUAV("OutputTexture", m_SATTexture->GetUAV());
	shader.BindParameters();

	UINT NumGroupsX = (UINT)ceilf(ShadowSize / 256.0f);
	UINT NumGroupsY = ShadowSize;
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);

	// Set PSO and RootSignature
	auto HorzSATPSO = PSOManager::m_ComputePSOMap["HorzSAT"];
	auto HorzSATShader = PSOManager::m_shaderMap["HorzSAT"];
	// Set Shader Parameter
	computeCmdList->SetComputeRootSignature(HorzSATPSO.GetRootSignature());
	computeCmdList->SetPipelineState(HorzSATPSO.GetPSO());

	HorzSATShader.SetParameter("InputTexture", m_ShadowMap->GetSRV());
	HorzSATShader.SetParameterUAV("OutputTexture", m_SATMiddleTexture->GetUAV());
	HorzSATShader.BindParameters();

	NumGroupsX = (UINT)ceilf(ShadowSize / 256.0f);
	NumGroupsY = ShadowSize;
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);

	g_CommandContext.Transition(m_SATMiddleTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(m_SATTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	auto VertSATPSO = PSOManager::m_ComputePSOMap["VertSAT"];
	auto VertSATShader = PSOManager::m_shaderMap["VertSAT"];
	// Set Shader Parameter
	computeCmdList->SetComputeRootSignature(HorzSATPSO.GetRootSignature());
	computeCmdList->SetPipelineState(HorzSATPSO.GetPSO());
	// Set Shader Parameter
	VertSATShader.SetParameter("InputTexture", m_SATMiddleTexture->GetSRV());
	VertSATShader.SetParameterUAV("OutputTexture", m_SATTexture->GetUAV());
	VertSATShader.BindParameters();

	NumGroupsX = (UINT)ceilf(ShadowSize / 256.0f);
	NumGroupsY = ShadowSize;
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::CascadedShadowMapPass()
{
	// update frustum
	XMFLOAT3 lightDir = XMFLOAT3{ 10.0f, -10.0f, 10.0f};
	m_CascadedShadowMap->DivideFrustum(lightDir, g_Camera.GetNearZ(), g_Camera.GetFarZ());

	auto shader = PSOManager::m_shaderMap["preDepthPass"];
	auto pso = PSOManager::m_gfxPSOMap["preDepthPassPSO"];
	auto gfxCmdList = g_CommandContext.GetCommandList();

	for (int i = 0; i < m_CascadedShadowMap->GetCascadeCount(); ++i)
	{
		// set viewport and scissorRect
		gfxCmdList->RSSetViewports(1, m_CascadedShadowMap->GetViewport());
		gfxCmdList->RSSetScissorRects(1, m_CascadedShadowMap->GetScissorRect());

		// transition buffer state
		g_CommandContext.Transition(m_CascadedShadowMap->GetD3D12Resource(i), D3D12_RESOURCE_STATE_DEPTH_WRITE);
		// set render target
		gfxCmdList->ClearDepthStencilView(m_CascadedShadowMap->GetDSV(i), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);
		gfxCmdList->OMSetRenderTargets(0, nullptr, false, &m_CascadedShadowMap->GetDSV(i));

		// set PSO and RootSignature
		gfxCmdList->SetGraphicsRootSignature(pso.GetRootSignature());
		gfxCmdList->SetPipelineState(pso.GetPSO());

		// UpdateShadowCB
		PassCBuffer passCB;
		passCB.EyePosition = g_Camera.GetPosition3f();
		passCB.lightPos = ImGuiManager::lightPos;

		XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(m_CascadedShadowMap->GetLightView(i)));
		XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(m_CascadedShadowMap->GetLightProj(i)));
		XMVECTOR det;
		XMMATRIX invProj = XMMatrixInverse(&det, m_CascadedShadowMap->GetLightProj(i));
		XMStoreFloat4x4(&passCB.invProjMat, XMMatrixTranspose(invProj));

		auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));

		// draw all mesh
		for (int i = 0; i < 5; i++)
		{
			for (int j = 0; j < 5; j++)
			{
				ObjCBuffer obj;
				XMMATRIX translationMatrix = XMMatrixTranslation(-42.5 + i * 20, 5, -42.5 + j * 20);
				XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(translationMatrix));
				ModelManager::m_ModelMaps["Cylinder"].SetObjCBuffer(obj);
				DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["Cylinder"], shader, passCBufferRef);
			}
		}
		DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["floor"], shader, passCBufferRef);

		g_CommandContext.Transition(m_CascadedShadowMap->GetD3D12Resource(i), D3D12_RESOURCE_STATE_DEPTH_READ);
	}

}

void TRender::GenerateVSM()
{
	// --------------- Get depth sqaure ---------------
	g_CommandContext.ResetCommandList();

	auto computeCmdList = g_CommandContext.GetCommandList();

	// Set PSO and RootSignature
	auto vsmPSO = PSOManager::m_ComputePSOMap["GenerateVSM"];
	auto vsmShader = PSOManager::m_shaderMap["GenerateVSM"];
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// Set Shader Parameter
	computeCmdList->SetComputeRootSignature(vsmPSO.GetRootSignature());
	computeCmdList->SetPipelineState(vsmPSO.GetPSO());

	vsmShader.SetParameter("ShadowMap", m_ShadowMap->GetSRV());
	vsmShader.SetParameterUAV("VSM", m_VSMTexture->GetUAV());
	vsmShader.BindParameters();

	UINT NumGroupsX = (UINT)ceilf(ShadowSize / 16.0f);
	UINT NumGroupsY = (UINT)ceilf(ShadowSize / 16.0f);
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	// --------------- Gaussian Blur ---------------
#if 1
	// --------------- Horizontal Blur ---------------
	// Set PSO and RootSignature
	auto HorzBlurPSO = PSOManager::m_ComputePSOMap["HorzBlurVSM"];
	auto HorzBlurShader = PSOManager::m_shaderMap["HorzBlurVSM"];

	// Set PSO and RootSignature
	computeCmdList->SetComputeRootSignature(HorzBlurPSO.GetRootSignature());
	computeCmdList->SetPipelineState(HorzBlurPSO.GetPSO());
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(m_VSMBlurTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// Set Shader Parameter
	HorzBlurShader.SetParameter("InputTexture", m_VSMTexture->GetSRV());
	HorzBlurShader.SetParameterUAV("OutputTexture", m_VSMBlurTexture->GetUAV());
	HorzBlurShader.SetParameter("BlurCBuffer", GaussWeightsCBRef);
	HorzBlurShader.BindParameters();

	NumGroupsX = (UINT)ceilf(ShadowSize / 256.0f);
	NumGroupsY = ShadowSize;
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);


	g_CommandContext.Transition(m_VSMBlurTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

	// --------------- Vertical Blur ---------------
	// Set PSO and RootSignature
	auto VertBlurPSO = PSOManager::m_ComputePSOMap["VertBlurVSM"];
	auto VertBlurShader = PSOManager::m_shaderMap["VertBlurVSM"];

	// Set PSO and RootSignature
	computeCmdList->SetComputeRootSignature(VertBlurPSO.GetRootSignature());
	computeCmdList->SetPipelineState(VertBlurPSO.GetPSO());

	// Set Shader Parameter
	VertBlurShader.SetParameter("InputTexture", m_VSMBlurTexture->GetSRV());
	VertBlurShader.SetParameterUAV("OutputTexture", m_VSMTexture->GetUAV());
	VertBlurShader.SetParameter("BlurCBuffer", GaussWeightsCBRef);
	VertBlurShader.BindParameters();

	NumGroupsX = ShadowSize;
	NumGroupsY = (UINT)ceilf(ShadowSize / 256.0f);
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);

	g_CommandContext.Transition(m_VSMBlurTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

#endif
	g_CommandContext.ExecuteCommandLists();
}

void TRender::GenerateESM()
{
	// --------------- Get Exponential Shadow Mapping ---------------
	g_CommandContext.ResetCommandList();

	auto computeCmdList = g_CommandContext.GetCommandList();

	// Set PSO and RootSignature
	auto EsmPSO = PSOManager::m_ComputePSOMap["GenerateESM"];
	auto EsmShader = PSOManager::m_shaderMap["GenerateESM"];
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// Set Shader Parameter
	computeCmdList->SetComputeRootSignature(EsmPSO.GetRootSignature());
	computeCmdList->SetPipelineState(EsmPSO.GetPSO());

	EsmShader.SetParameter("ShadowMap", m_ShadowMap->GetSRV());
	EsmShader.SetParameterUAV("ESM", m_ESMTexture->GetUAV());
	EsmShader.BindParameters();

	UINT NumGroupsX = (UINT)ceilf(ShadowSize / 16.0f);
	UINT NumGroupsY = (UINT)ceilf(ShadowSize / 16.0f);
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);
	g_CommandContext.Transition(m_ESMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::GenerateEVSM()
{
	// --------------- Get Exponential Shadow Mapping ---------------
	g_CommandContext.ResetCommandList();

	auto computeCmdList = g_CommandContext.GetCommandList();

	// Set PSO and RootSignature
	auto EvsmPSO = PSOManager::m_ComputePSOMap["GenerateEVSM"];
	auto EvsmShader = PSOManager::m_shaderMap["GenerateEVSM"];
	g_CommandContext.Transition(m_VSMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	// Set Shader Parameter
	computeCmdList->SetComputeRootSignature(EvsmPSO.GetRootSignature());
	computeCmdList->SetPipelineState(EvsmPSO.GetPSO());

	EvsmShader.SetParameter("ShadowMap", m_ShadowMap->GetSRV());
	EvsmShader.SetParameterUAV("EVSM", m_EVSMTexture->GetUAV());
	EvsmShader.BindParameters();

	UINT NumGroupsX = (UINT)ceilf(ShadowSize / 16.0f);
	UINT NumGroupsY = (UINT)ceilf(ShadowSize / 16.0f);
	computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);
	g_CommandContext.Transition(m_ESMTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	g_CommandContext.ExecuteCommandLists();
}

void TRender::GenerateVSSM()
{
	// --------------- Get depth sqaure ---------------
	g_CommandContext.ResetCommandList();

	auto computeCmdList = g_CommandContext.GetCommandList();

	// Set PSO and RootSignature
	auto vsmPSO = PSOManager::m_ComputePSOMap["GenerateVSM"];
	auto vsmShader = PSOManager::m_shaderMap["GenerateVSM"];

	for (int mip = 0; mip < VSSMMaxRadius; mip++)
	{
		// update Gaussian Weights
		auto GaussWeights = CalcGaussianWeights(float(mip+1)/2.0f);
		int BlurRaidus = (int)GaussWeights.size() / 2;

		BlurCBuffer blurCB;
		blurCB.gBlurRadius = BlurRaidus;
		size_t DataSize = GaussWeights.size() * sizeof(float);
		memcpy_s(&blurCB.w0, DataSize, GaussWeights.data(), DataSize);
		auto gaussWeightsCBRef = TD3D12RHI::CreateConstantBuffer(&blurCB, sizeof(BlurCBuffer));

		g_CommandContext.Transition(m_VSSMTextures[mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// Set Shader Parameter
	
		computeCmdList->SetComputeRootSignature(vsmPSO.GetRootSignature());
		computeCmdList->SetPipelineState(vsmPSO.GetPSO());
		vsmShader.SetParameter("ShadowMap", m_ShadowMap->GetSRV());
		vsmShader.SetParameterUAV("VSM", m_VSSMTextures[mip]->GetUAV());
		vsmShader.BindParameters();

		UINT NumGroupsX = (UINT)ceilf(ShadowSize / 16.0f);
		UINT NumGroupsY = (UINT)ceilf(ShadowSize / 16.0f);
		computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);


		g_CommandContext.Transition(m_VSSMTextures[mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

		// --------------- Gaussian Blur ---------------
		// --------------- Horizontal Blur ---------------
		// Set PSO and RootSignature
		auto HorzBlurPSO = PSOManager::m_ComputePSOMap["HorzBlurVSM"];
		auto HorzBlurShader = PSOManager::m_shaderMap["HorzBlurVSM"];
		// Set PSO and RootSignature
		computeCmdList->SetComputeRootSignature(HorzBlurPSO.GetRootSignature());
		computeCmdList->SetPipelineState(HorzBlurPSO.GetPSO());
		g_CommandContext.Transition(m_VSSMTextures[mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		g_CommandContext.Transition(m_VSMBlurTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		// Set Shader Parameter
		HorzBlurShader.SetParameter("InputTexture", m_VSSMTextures[mip]->GetSRV());
		HorzBlurShader.SetParameterUAV("OutputTexture", m_VSMBlurTexture->GetUAV());
		HorzBlurShader.SetParameter("BlurCBuffer", gaussWeightsCBRef);
		HorzBlurShader.BindParameters();

		NumGroupsX = (UINT)ceilf(ShadowSize / 256.0f);
		NumGroupsY = ShadowSize;
		computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);

		g_CommandContext.Transition(m_VSMBlurTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		g_CommandContext.Transition(m_VSSMTextures[mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

		// --------------- Vertical Blur ---------------
		// Set PSO and RootSignature
		auto VertBlurPSO = PSOManager::m_ComputePSOMap["VertBlurVSM"];
		auto VertBlurShader = PSOManager::m_shaderMap["VertBlurVSM"];

		// Set PSO and RootSignature
		computeCmdList->SetComputeRootSignature(VertBlurPSO.GetRootSignature());
		computeCmdList->SetPipelineState(VertBlurPSO.GetPSO());

		// Set Shader Parameter
		VertBlurShader.SetParameter("InputTexture", m_VSMBlurTexture->GetSRV());
		VertBlurShader.SetParameterUAV("OutputTexture", m_VSSMTextures[mip]->GetUAV());
		VertBlurShader.SetParameter("BlurCBuffer", gaussWeightsCBRef);
		VertBlurShader.BindParameters();

		NumGroupsX = ShadowSize;
		NumGroupsY = (UINT)ceilf(ShadowSize / 256.0f);
		computeCmdList->Dispatch(NumGroupsX, NumGroupsY, 1);

		g_CommandContext.Transition(m_VSMBlurTexture->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);
		g_CommandContext.Transition(m_VSSMTextures[mip]->GetD3D12Resource(), D3D12_RESOURCE_STATE_GENERIC_READ);

	}
	g_CommandContext.ExecuteCommandLists();
}

TD3D12ConstantBufferRef TRender::UpdatePassCbuffer()
{
	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(g_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(g_Camera.GetProjMat()));
	XMStoreFloat4x4(&passCB.invProjMat, XMMatrixTranspose(g_Camera.GetInvProjMat()));
	passCB.EyePosition = g_Camera.GetPosition3f();
	passCB.lightPos = ImGuiManager::lightPos;
	passCB.lightColor = ImGuiManager::lightColor;
	passCB.Intensity = ImGuiManager::Intensity;
	passCB.ScreenDimensions = { (float)g_DisplayWidth, (float)g_DisplayHeight };
	passCB.InvScreenDimensions = { 1.0f / (float)g_DisplayWidth, 1.0f / (float)g_DisplayHeight };
	auto passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));

	return passCBufferRef;
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

void TRender::CreateGBuffersResource()
{
	GBufferAlbedo = std::make_unique<RenderTarget2D>(L"Gbuffer Albedo", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GBufferSpecular = std::make_unique<RenderTarget2D>(L"Gbuffer Specular", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GBufferWorldPos = std::make_unique<RenderTarget2D>(L"Gbuffer WorldPosition", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);
	GBufferNormal = std::make_unique<RenderTarget2D>(L"Gbuffer Normal", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16G16B16A16_FLOAT);

	GBufferDepth = std::make_unique<D3D12DepthBuffer>();
	GBufferDepth->Create(L"Gbuffer Depth", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_D32_FLOAT);


	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);

	struct TileLightInfo
	{
		UINT LightIndices[100];
		UINT LightCount;
	};

	TileLightInfoListRef = TD3D12RHI::CreateRWStructuredBuffer(sizeof(TileLightInfo), numGroupsX * numGroupsY);
	

	TiledDepthDebugTexture = std::make_unique<D3D12ColorBuffer>();
	TiledDepthDebugTexture->Create(L"TiledDepthDebugTexture", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R32G32_FLOAT);

	// SSAO Map
	SSAOTexture = std::make_unique<D3D12ColorBuffer>();
	SSAOTexture->Create(L"SSAO Texture", g_DisplayWidth, g_DisplayHeight, 1, DXGI_FORMAT_R16_FLOAT);
}

void TRender::CreateGBuffersPSO()
{
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
	//GBuffersPso.SetDepthTargetFormat(GBufferDepth->GetFormat());
	DXGI_FORMAT GBuffersFormat[] =
	{
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		DXGI_FORMAT_R16G16B16A16_FLOAT,
		//DXGI_FORMAT_R16G16B16A16_FLOAT
	};
	GBuffersPso.SetRenderTargetFormats(_countof(GBuffersFormat), GBuffersFormat, GBufferDepth->GetFormat());
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
	DeferredShadingPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, GBufferDepth->GetFormat());
	DeferredShadingPso.Finalize();
	m_gfxPSOMap["DeferredShadingPso"] = DeferredShadingPso;
}

void TRender::CreateForwardPulsResource()
{
	UINT numGroupsX = (UINT)ceilf(g_DisplayWidth / 16.0f);
	UINT numGroupsY = (UINT)ceilf(g_DisplayHeight / 16.0f);

	DebugMap = std::make_unique<D3D12ColorBuffer>();
	DebugMap->Create(L"Debug Map", numGroupsX * 16, numGroupsY * 16, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);
}

void TRender::CreateShadowResource()
{
	m_ShadowMap = std::make_unique<ShadowMap>(L"ShadowMap", ShadowSize, ShadowSize, DXGI_FORMAT_D32_FLOAT);

	XMFLOAT3 lightDir = XMFLOAT3{ 10.0f, -10.0f, 10.0f};
	m_ShadowMap->SetLightView(lightDir);
	// directional light
	std::vector<LightInfo> directionalLights;
	LightInfo directionalLight;
	directionalLight.Type = ELightType::DirectionalLight;
	directionalLight.Color = XMFLOAT4{ 1.0, 1.0, 1.0, 1.0 };
	directionalLight.DirectionW = XMFLOAT4{ lightDir.x,lightDir.y,lightDir.z, 1.0f };
	directionalLight.Intensity = 5;
	XMStoreFloat4x4(&directionalLight.ShadowTransform, XMMatrixTranspose(m_ShadowMap->GetShadowTransform()));
	directionalLights.push_back(directionalLight);

	LightManager::DirectionalLight.SetLightInfo(directionalLights);
	LightManager::DirectionalLight.CreateStructuredBufferRef();

	// Create VSM Shadow Buffer
	m_VSMTexture = std::make_unique<D3D12ColorBuffer>();
	m_VSMTexture->Create(L"VSM Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32G32_FLOAT);

	m_VSMBlurTexture = std::make_unique<D3D12ColorBuffer>();
	m_VSMBlurTexture->Create(L"VSM Blur Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32G32_FLOAT);

	// Create gaussian blur weights
	auto GaussWeights = CalcGaussianWeights(1.5f);
	int BlurRaidus = (int)GaussWeights.size() / 2;

	BlurCBuffer blurCB;
	blurCB.gBlurRadius = BlurRaidus;
	size_t DataSize = GaussWeights.size() * sizeof(float);
	memcpy_s(&blurCB.w0, DataSize, GaussWeights.data(), DataSize);

	GaussWeightsCBRef = TD3D12RHI::CreateConstantBuffer(&blurCB, sizeof(BlurCBuffer));

	// Create ESM
	m_ESMTexture = std::make_unique<D3D12ColorBuffer>();
	m_ESMTexture->Create(L"ESM Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32_FLOAT);

	// Create EVSM
	m_EVSMTexture = std::make_unique<D3D12ColorBuffer>();
	m_EVSMTexture->Create(L"EVSM Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32G32B32A32_FLOAT);

	// Create SAT
	m_SATTexture = std::make_unique<D3D12ColorBuffer>();
	m_SATTexture->Create(L"SAT Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32_FLOAT);

	m_SATMiddleTexture = std::make_unique<D3D12ColorBuffer>();
	m_SATMiddleTexture->Create(L"SAT Middle Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32_FLOAT);

	for (int Mip = 0; Mip < VSSMMaxRadius; Mip++)
	{
		auto Texture = std::make_unique<D3D12ColorBuffer>();
		Texture->Create(L"VSSM Texture", ShadowSize, ShadowSize, 1, DXGI_FORMAT_R32G32_FLOAT);
		m_VSSMTextures.push_back(std::move(Texture));

	}
}

void TRender::CreateCSMResource()
{
	
	m_CascadedShadowMap = std::make_unique<CascadedShadowMap>(L"Cascaded ShadowMap", CSMSize, CSMSize, DXGI_FORMAT_D32_FLOAT, CSM_MAX_COUNT);
}

std::vector<float> TRender::CalcGaussianWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	int blurRadius = (int)ceil(2.0f * sigma);

	const int MaxBlurRadius = 5;
	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;
		weights[i + blurRadius] = expf(-x * x / twoSigma2);
		weightSum += weights[i + blurRadius];
	}

	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}
