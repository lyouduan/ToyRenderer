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
#include "Light.h"

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
	XMMATRIX scalingMat = XMMatrixScaling(ImGuiManager::scale * 0.5, ImGuiManager::scale * 0.5, ImGuiManager::scale * 0.5);

	totalTime += gt.DeltaTime() * 0.01;
	float rotate_angle = static_cast<float>(ImGuiManager::RotationY * 360  * totalTime);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
	XMMATRIX rotationYMat = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(rotate_angle));
	XMMATRIX translationMatrix = XMMatrixTranslation(ImGuiManager::modelPosition.x, ImGuiManager::modelPosition.y, ImGuiManager::modelPosition.z);

	m_ModelMatrix = scalingMat * rotationYMat * translationMatrix;

	// save previous camera datas
	XMMATRIX view = g_Camera.GetViewMat();
	XMMATRIX proj = g_Camera.GetProjMat();
	g_Camera.SetPreViewProj(view * proj);

	// update
	g_Camera.UpdateViewMat();

	ObjCBuffer obj;
	XMStoreFloat4x4(&obj.ModelMat, XMMatrixTranspose(m_ModelMatrix));
	objCBufferRef = CreateConstantBuffer(&obj, sizeof(ObjCBuffer));

	ObjCBuffer objCB;
	XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(m_ModelMatrix));
	ModelManager::m_ModelMaps["nanosuit"].SetObjCBuffer(objCB);
	
	//memcpy(passCBufferRef->ResourceLocation.MappedAddress, &passCB, sizeof(PassCBuffer));

	XMMATRIX m_LightMatrix = XMMatrixTranslation(ImGuiManager::lightPos.x, ImGuiManager::lightPos.y, ImGuiManager::lightPos.z);
	ObjCBuffer lightObj;
	XMStoreFloat4x4(&lightObj.ModelMat, XMMatrixTranspose(m_LightMatrix));
	lightObjCBufferRef = CreateConstantBuffer(&lightObj, sizeof(ObjCBuffer));
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

	m_Render->EndFrame();
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
		// model and light 
		ImGuiManager::RenderAllItem();

		ImGui::NewLine();
		
		if(ImGui::Checkbox("UseEquirectangularMap", &ImGuiManager::useCubeMap))
		{
			if (ImGuiManager::useCubeMap)
			{
				m_Render->SetbUseEquirectangularMap(true);
				m_Render->CreateIBLIrradianceMap();
				m_Render->CreateIBLPrefilterMap();
			}
			else
			{
				m_Render->SetbUseEquirectangularMap(false);
				m_Render->CreateIBLIrradianceMap();
				m_Render->CreateIBLPrefilterMap();
			}
		}

		// GBuffer ImGui
		if (ImGui::Checkbox("EnableDeferredRendering", &ImGuiManager::bEnableDeferredRendering))
		{
			if (ImGuiManager::bEnableDeferredRendering)
				m_Render->SetEnableDeferredRendering(true);
			else
				m_Render->SetEnableDeferredRendering(false);
		}

		if (ImGuiManager::bEnableDeferredRendering)
		{
			if (ImGui::Checkbox("Debug GBuffers", &ImGuiManager::bDebugGBuffers))
			{
				if (ImGuiManager::bDebugGBuffers)
					m_Render->SetbDebugGBuffers(true);
				else
					m_Render->SetbDebugGBuffers(false);
			}

			if (ImGuiManager::bDebugGBuffers)
				ImGuiManager::RenderCombo();

			if (ImGui::Checkbox("Enable TAA", &ImGuiManager::bEnableTAA))
			{
				if (ImGuiManager::bEnableTAA)
				{
					m_Render->SetbEnableTAA(true);
				}
				else
				{
					m_Render->SetbEnableTAA(false);
				}
			}
		}
		
		

		// forward puls
		if (ImGui::Checkbox("Enable Forward Puls Rendering", &ImGuiManager::bEnableForwardPuls))
		{
			if (ImGuiManager::bEnableForwardPuls)
				m_Render->SetEnableForwardPulsPass(true);
			else
				m_Render->SetEnableForwardPulsPass(false);
		}

		// shadow map
		if (ImGui::Checkbox("Enable ShadowMap", &ImGuiManager::bEnableShadowMap))
		{
			if (ImGuiManager::bEnableShadowMap)
				m_Render->SetbEnableShadowMap(true);
			else
				m_Render->SetbEnableShadowMap(false);
		}

		if (ImGuiManager::bEnableShadowMap)
			ImGuiManager::ShadowTypeCombo();


		// Post processing FXAA
		if (ImGui::Checkbox("Enable FXAA", &ImGuiManager::bEnableFXAA))
		{
			if (ImGuiManager::bEnableFXAA)
			{
				m_Render->SetbEnableFXAA(true);
			}
			else
			{
				m_Render->SetbEnableFXAA(false);
			}
		}

		// camera control
		g_Camera.CamerImGui();

		ImGui::End();
	}
	// Rendering ImGui
	ImGui::Render();

}

void GameCore::LoadPipeline()
{
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
	g_Camera.SetPosition(0, 5, -20);

	// load model
	ModelManager::LoadModel();

	// load Texture
	TextureManager::LoadTexture();

	// Load Lights
	LightManager::InitialzeLights();

	m_Render = std::make_unique<TRender>();
	m_Render->Initalize();
	m_Render->CreateIBLEnvironmentMap();
	m_Render->CreateIBLIrradianceMap();
	m_Render->CreateIBLPrefilterMap();
	m_Render->CreateIBLLUT2D();

	// Shadow Map
	m_Render->ShadowPass();
	m_Render->GenerateVSM();
	m_Render->GenerateESM();
	m_Render->GenerateEVSM();
	m_Render->GenerateVSSM();

	m_Render->GenerateSAT();

	g_CommandContext.FlushCommandQueue();
}

void GameCore::PopulateCommandList()
{
	// before reset a allocator, all commandlist associated with the allocator have completed
	g_CommandContext.FlushCommandQueue();
	// reset CommandAllocator and CommandList
	g_CommandContext.ResetCommandAllocator();
	g_CommandContext.ResetCommandList();

	m_Render->SetDescriptorHeaps();
	
	if((ImGuiManager::ShadowType) == (int)ShadowType::CSM)
		m_Render->CascadedShadowMapPass();

	if (m_Render->GetEnableDeferredRendering() || m_Render->GetbDebugGBuffers())
	{
		m_Render->GbuffersPass();
		m_Render->SSAOPass();
		m_Render->HBAOPass();
		m_Render->TiledBaseLightCulling();
	}

	if (m_Render->GetEnableForwardPulsPass())
	{
		m_Render->PrePassDepthBuffer();
		m_Render->CullingLightPass();
	}
	
	// set necessary state
	g_CommandContext.GetCommandList()->RSSetViewports(1, &m_viewport);
	g_CommandContext.GetCommandList()->RSSetScissorRects(1, &m_scissorRect);

	// indicate that back buffer will be used as a render target
	g_CommandContext.Transition(m_renderTragetrs[g_frameIndex].GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// indicate that back buffer will be used as a render target
	g_CommandContext.GetCommandList()->ClearRenderTargetView(m_renderTragetrs[g_frameIndex].GetRTV(), (FLOAT*)ImGuiManager::clearColor, 0, nullptr);
	g_CommandContext.GetCommandList()->ClearDepthStencilView(TD3D12RHI::g_DepthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

	g_CommandContext.GetCommandList()->OMSetRenderTargets(1, &m_renderTragetrs[g_frameIndex].GetRTV(), TRUE, &TD3D12RHI::g_DepthBuffer.GetDSV());


	// full quad
	if (m_Render->GetEnableDeferredRendering())
	{
		m_Render->DeferredShadingPass();
		
		if (m_Render->GetbDebugGBuffers())
			m_Render->GbuffersDebug();

		if (m_Render->GetbEnableTAA())
			m_Render->TAAPass();
	}
	else if (m_Render->GetEnableForwardPulsPass())
	{
		m_Render->ForwardPlusPass();
		m_Render->LightGridDebug();
		// light pass
		m_Render->LightPass();

	}
	else if (m_Render->GetbEnableShadowMap())
	{
		m_Render->ScenePass();
		m_Render->ShadowMapDebug();
	}
	else
	{
		m_Render->IBLRenderPass();
	}

	
	if(m_Render->GetbEnableFXAA())
		m_Render->FXAAPass();

	// Draw ImGui
	ImGuiManager::EndRenderImGui(g_CommandContext);

	// indicate that the back buffer will now be used to present
	g_CommandContext.Transition(m_renderTragetrs[g_frameIndex].GetD3D12Resource(), D3D12_RESOURCE_STATE_PRESENT);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);

}

void GameCore::WaitForPreviousFrame()
{
	g_CommandContext.FlushCommandQueue();
	
	g_DescriptorCache->Reset();

	g_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
