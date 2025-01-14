#include "GameCore.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12RHI.h"
#include "D3D12Texture.h"
#include "Display.h"
#include <chrono>
#include "ImGuiManager.h"
#include "PSOManager.h"
#include "ModelManager.h"
#include "TextureManager.h"
#include "SceneCaptureCube.h"
#include "RenderInfo.h"

using namespace TD3D12RHI;
using TD3D12RHI::g_CommandContext;
using namespace PSOManager;

GameCore::GameCore(uint32_t width, uint32_t height, std::wstring name)
	:DXSample(width, height, name),
	m_viewport(0.0f,0.0f, static_cast<float>(width), static_cast<float>(height)),
	m_scissorRect(0,0, static_cast<LONG>(width), static_cast<LONG>(height))
{
	g_DisplayWidth = width;
	g_DisplayHeight = height;
}

void GameCore::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

void GameCore::OnUpdate(const GameTimer& gt)
{
	XMMATRIX scalingMat = XMMatrixScaling(scale * 0.5, scale* 0.5, scale * 0.5);

	totalTime += gt.DeltaTime() * 0.01;
	float rotate_angle = static_cast<float>(RotationY * 360  * totalTime);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
	XMMATRIX rotationYMat = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(rotate_angle));
	XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y+5, position.z);

	m_ModelMatrix = scalingMat * rotationYMat * translationMatrix;

	// update camera
	m_Camera.UpdateViewMat();

	ObjCBuffer obj;
	XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(m_ModelMatrix));
	objCBufferRef = CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

	ObjCBuffer objCB;
	XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(m_ModelMatrix));
	ModelManager::m_ModelMaps["nanosuit"].SetObjCBuffer(objCB);

	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(m_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(m_Camera.GetProjMat()));


	passCB.lightPos = lightPos;
	passCB.EyePosition = m_Camera.GetPosition3f();
	passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));
	//memcpy(passCBufferRef->ResourceLocation.MappedAddress, &passCB, sizeof(PassCBuffer));

	XMMATRIX m_LightMatrix = XMMatrixTranslation(lightPos.x, lightPos.y, lightPos.z);
	ObjCBuffer lightObj;
	XMStoreFloat4x4(&lightObj.ModelMat, XMMatrixTranspose(m_LightMatrix));
	lightObjCBufferRef = CreateConstantBuffer(&lightObj, sizeof(ObjCBuffer));

	// mat CBuffer
	matCBufferRef = TD3D12RHI::CreateConstantBuffer(&matCB, sizeof(MatCBuffer));
}

void GameCore::OnRender()
{
	UpdateImGui();

	// record all the commands we need to render the scene into the command list
	PopulateCommandList();

	// execute the command list
	g_CommandContext.ExecuteCommandLists();

	// present the frame
	ThrowIfFailed(m_swapChain->Present(1, 0));

	WaitForPreviousFrame();
} 

void GameCore::OnDestroy()
{
	// Ensure that the GPU is no longer referencing resources that are about to be
	// cleaned up by the destructor.
	WaitForPreviousFrame();

	// destroy ImGui
	ImGuiManager::DestroyImGui();

	ModelManager::DestroyModel();

	TextureManager::DestroyTexture();
}

void GameCore::UpdateImGui()
{
	// Start the Dear ImGui frame
	ImGuiManager::StartRenderImGui();
	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	{
		ImGui::Begin("ImGui!");                          // Create a window called "ImGui!" and append into it.
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		// Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &ImGuiManager::show_demo_window);      // Edit bools storing our window open/close state
		//ImGui::Checkbox("Another Window", &show_another_window);
		ImGui::Text("Model Control Parameters");
		ImGui::SliderFloat("RotationY", &RotationY, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Scale", &scale, 0.0f, 10.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Model PositionX", &position.x, -20.0f, 20.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Model PositionY", &position.y, -20.0f, 20.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Model PositionZ", &position.z, -20.0f, 20.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

		ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color

		if(ImGui::Checkbox("EquirectangularMap", &ImGuiManager::useCubeMap))
		{
			if(ImGuiManager::useCubeMap)
				m_Render->SetEnableIBLEnvLighting(true);
			else
				m_Render->SetEnableIBLEnvLighting(false);
		}
		//if (ImGui::Button("CubeMap"))                          // Buttons return true when clicked (most widgets return true when edited/activated)
		//counter++;
	//ImGui::SameLine();
	//ImGui::Text("counter = %d", counter);

	// Buffer
	//ImGui::InputText("Butts", buffer, sizeof(buffer));

	// camera control
		m_Camera.CamerImGui();

		ImGui::NewLine();
		ImGui::Text("Light Position");
		ImGui::SliderFloat3("lightPos", (float*)&lightPos, -300, 300);
		ImGui::NewLine();

		ImGui::Text("Model PBR");
		ImGui::ColorEdit4("baseColor", (float*)&matCB.DiffuseAlbedo);
		ImGui::SliderFloat("Roughness", &matCB.Roughness, 0.0f, 1.0f);
		ImGui::SliderFloat("Metallic", &matCB.metallic, 0.0f, 1.0f);

		ImGui::End();
	}
	// Rendering ImGui
	ImGui::Render();

}

void GameCore::DrawMesh(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader)
{
	auto obj = model.GetObjCBuffer();
	auto objCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

	auto meshes = model.GetMeshes();
	for (UINT i = 0; i < meshes.size(); ++i)
	{
		shader.SetParameter("objCBuffer", objCBufferRef);
		shader.SetParameter("passCBuffer", passCBufferRef);
		// draw call
		auto SRV = meshes[i].GetSRV();
		if (!SRV.empty())
			shader.SetParameter("diffuseMap", SRV[0]);
		else
		{
			shader.SetParameter("diffuseMap", NullDescriptor);
		}
		//m_shader->SetParameter("specularMap", SRV[1]);
		//m_shader->SetParameter("normalMap", SRV[2]);

		shader.SetDescriptorCache(meshes[i].GetTD3D12DescriptorCache());
		shader.BindParameters(); // after binding Parameter, it will clear all Parameter
		meshes[i].DrawMesh(g_CommandContext);
	}
}

void GameCore::LoadPipeline()
{
//	uint32_t dxgiFactoryFlags = 0;
//#if defined(_DEBUG)
//	{
//		ComPtr<ID3D12Debug> debugController;
//		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
//		{
//			debugController->EnableDebugLayer();
//			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
//		}
//	}
//#endif

	uint32_t useDebugLayers = 0;
#if _DEBUG
	// Default to true for debug builds
	useDebugLayers = 1;
#endif

	DWORD dxgiFactoryFlags = 0;

	if (useDebugLayers)
	{
		Microsoft::WRL::ComPtr<ID3D12Debug> debugInterface;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
		{
			debugInterface->EnableDebugLayer();

			uint32_t useGPUBasedValidation = 0;
			if (useGPUBasedValidation)
			{
				Microsoft::WRL::ComPtr<ID3D12Debug1> debugInterface1;
				if (SUCCEEDED((debugInterface->QueryInterface(IID_PPV_ARGS(&debugInterface1)))))
				{
					debugInterface1->SetEnableGPUBasedValidation(true);
				}
			}
		}
		else
		{
			OutputDebugStringA("WARNING:  Unable to enable D3D12 debug validation layer\n");
		}

#if _DEBUG
		ComPtr<IDXGIInfoQueue> dxgiInfoQueue;
		if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))))
		{
			dxgiFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;

			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
			dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);

			DXGI_INFO_QUEUE_MESSAGE_ID hide[] =
			{
				80 /* IDXGISwapChain::GetContainingOutput: The swapchain's adapter does not control the output on which the swapchain's window resides. */,
			};
			DXGI_INFO_QUEUE_FILTER filter = {};
			filter.DenyList.NumIDs = _countof(hide);
			filter.DenyList.pIDList = hide;
			dxgiInfoQueue->AddStorageFilterEntries(DXGI_DEBUG_DXGI, &filter);
		}
#endif
	}

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (m_useWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(
			warpAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);
		
		ThrowIfFailed(D3D12CreateDevice(
			hardwareAdapter.Get(),
			D3D_FEATURE_LEVEL_11_0,
			IID_PPV_ARGS(&m_device)));
	}
	TD3D12RHI::g_Device = m_device.Detach();

	// using device to initialize CommandContext and allocator
	TD3D12RHI::Initialze();

	//descriptorCache = std::make_unique<TD3D12DescriptorCache>(g_Device);

	// create the swap chain
	Display::Initialize();

	// this sample does not support fullscreen transition
	ThrowIfFailed(factory->MakeWindowAssociation(Win32Application::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(TD3D12RHI::g_SwapCHain.As(&m_swapChain));
	g_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

	// create frame resources
	{
		for(uint32_t n = 0; n < FrameCount; ++n)
		{
			ComPtr<ID3D12Resource> resource = nullptr;
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&resource)));
			m_renderTragetrs[n].CreateFromSwapChain(L"Render Target", resource.Detach());
		}
	}
}

// load the sample assets
void GameCore::LoadAssets()
{
	// create PSO and rootSignature
	PSOManager::InitializePSO();

	// set camera
	m_Camera.SetPosition(0, 5, -20);

	// load model
	ModelManager::LoadModel();
	ModelManager::LoadMesh();

	// load Texture
	TextureManager::LoadTexture();


	// Render equirectangularMap To CubeMap
	//m_RenderCubeMap = std::make_shared<SceneCaptureCube>(L"CubeMap", 256, 256, 1, DXGI_FORMAT_R8G8B8A8_UNORM);
	//m_RenderCubeMap->CreateCubeCamera({ 0, 0, 0 }, 0.1, 10);
	//m_RenderCubeMap->DrawEquirectangularMapToCubeMap(g_CommandContext);
	m_Render = std::make_unique<TRender>();
	m_Render->Initalize();
	
	m_Render->CreateIBLEnvironmentMap();
	m_Render->CreateIBLIrradianceMap();
	m_Render->CreateIBLPrefilterMap();
	m_Render->CreateIBLLUT2D();

	g_CommandContext.FlushCommandQueue();
}

void GameCore::PopulateCommandList()
{
	// before reset a allocator, all commandlist associated with the allocator have completed
	g_CommandContext.FlushCommandQueue();
	// reset CommandAllocator and CommandList
	g_CommandContext.ResetCommandAllocator();
	g_CommandContext.ResetCommandList();

	//if(g_frameIndex == 0)

	// set necessary state
	g_CommandContext.GetCommandList()->RSSetViewports(1, &m_viewport);
	g_CommandContext.GetCommandList()->RSSetScissorRects(1, &m_scissorRect);

	// indicate that back buffer will be used as a render target
	g_CommandContext.Transition(m_renderTragetrs[g_frameIndex].GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// indicate that back buffer will be used as a render target
	g_CommandContext.GetCommandList()->ClearRenderTargetView(m_renderTragetrs[g_frameIndex].GetRTV(), (FLOAT*)clearColor, 0, nullptr);
	g_CommandContext.GetCommandList()->ClearDepthStencilView(TD3D12RHI::g_DepthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

	g_CommandContext.GetCommandList()->OMSetRenderTargets(1, &m_renderTragetrs[g_frameIndex].GetRTV(), TRUE, &TD3D12RHI::g_DepthBuffer.GetDSV());

	// Record commands
	//g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["pso"].GetRootSignature());
	//g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["pso"].GetPSO());
	//DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["nanosuit"], m_shaderMap["modelShader"]);
	//DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["wall"], m_shaderMap["modelShader"]);

	// full quad
	
	{
		g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["quadPSO"].GetRootSignature());
		g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["quadPSO"].GetPSO());
		ObjCBuffer obj;
		objCBufferRef = CreateConstantBuffer(&obj, sizeof(ObjCBuffer));


		m_shaderMap["quadShader"].SetDescriptorCache(ModelManager::m_MeshMaps["FullQuad"].GetTD3D12DescriptorCache());
		auto depthSrv = m_Render->GetIBLBrdfLUT2D()->GetSRV();
		m_shaderMap["quadShader"].SetParameter("tex", depthSrv);
		m_shaderMap["quadShader"].BindParameters();
		ModelManager::m_MeshMaps["FullQuad"].DrawMesh(g_CommandContext);
	}

	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["pbrPSO"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["pbrPSO"].GetPSO());
	
	m_shaderMap["pbrShader"].SetParameter("objCBuffer", objCBufferRef);
	m_shaderMap["pbrShader"].SetParameter("matCBuffer", matCBufferRef);
	m_shaderMap["pbrShader"].SetParameter("passCBuffer", passCBufferRef);
	m_shaderMap["pbrShader"].SetParameter("IrradianceMap", m_Render->GetIBLIrradianceMap()->GetSRV());
	m_shaderMap["pbrShader"].SetDescriptorCache(ModelManager::m_MeshMaps["sphere"].GetTD3D12DescriptorCache());
	m_shaderMap["pbrShader"].BindParameters();
	ModelManager::m_MeshMaps["sphere"].DrawMesh(g_CommandContext);

	// light 
	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["lightPSO"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["lightPSO"].GetPSO());
	m_shaderMap["lightShader"].SetParameter("objCBuffer", lightObjCBufferRef);
	m_shaderMap["lightShader"].SetParameter("passCBuffer", passCBufferRef);
	m_shaderMap["lightShader"].SetDescriptorCache(ModelManager::m_MeshMaps["sphere"].GetTD3D12DescriptorCache());
	m_shaderMap["lightShader"].BindParameters();
	ModelManager::m_MeshMaps["sphere"].DrawMesh(g_CommandContext);

	// sky box
	{
		g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["skyboxPSO"].GetRootSignature());
		g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["skyboxPSO"].GetPSO());

		m_shaderMap["skyboxShader"].SetParameter("objCBuffer", objCBufferRef); // don't need objCbuffer
		m_shaderMap["skyboxShader"].SetParameter("passCBuffer", passCBufferRef);

		if (m_Render->GetEnableIBLEnvLighting())
			m_shaderMap["skyboxShader"].SetParameter("CubeMap", m_Render->GetIBLPrefilterMaps(0)->GetSRV());
		else
			m_shaderMap["skyboxShader"].SetParameter("CubeMap", TextureManager::m_SrvMaps["skybox"]);
		//m_shaderMap["skyboxShader"].SetParameter("CubeMap", m_Render->GetIBLIrradianceMap()->GetSRV());

		m_shaderMap["skyboxShader"].SetDescriptorCache(ModelManager::m_MeshMaps["box"].GetTD3D12DescriptorCache());
		m_shaderMap["skyboxShader"].BindParameters();
		ModelManager::m_MeshMaps["box"].DrawMesh(g_CommandContext);
	}

	// Draw ImGui
	ImGuiManager::EndRenderImGui(g_CommandContext);

	// indicate that the back buffer will now be used to present
	g_CommandContext.Transition(m_renderTragetrs[g_frameIndex].GetD3D12Resource(), D3D12_RESOURCE_STATE_PRESENT);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);

}

void GameCore::WaitForPreviousFrame()
{
	g_CommandContext.FlushCommandQueue();
	
	DescriptorCache->Reset();

	g_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
