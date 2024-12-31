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

using namespace TD3D12RHI;
using TD3D12RHI::g_CommandContext;
using namespace PSOManager;

GameCore::GameCore(uint32_t width, uint32_t height, std::wstring name)
	:DXSample(width, height, name),
	m_frameIndex(0),
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
	XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

	m_ModelMatrix = scalingMat * rotationYMat * translationMatrix;
	
	m_Camera.UpdateViewMat();
	//m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	//float aspectRatio = GetWidth() / static_cast<float>(GetHeight());
	//m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0), aspectRatio, 0.1f, 100.0f);
	//m_Camera.SetAspectRatio(aspectRatio);

	ObjCBuffer objCB;
	XMStoreFloat4x4(&objCB.ModelMat, XMMatrixTranspose(m_ModelMatrix));
	ModelManager::m_ModelMaps["nanosuit"].SetObjCBuffer(objCB);

	PassCBuffer passCB;
	XMStoreFloat4x4(&passCB.ViewMat, XMMatrixTranspose(m_Camera.GetViewMat()));
	XMStoreFloat4x4(&passCB.ProjMat, XMMatrixTranspose(m_Camera.GetProjMat()));

	passCB.EyePosition = m_Camera.GetPosition3f();
	passCBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PassCBuffer));
}

void GameCore::OnRender()
{
	// Start the Dear ImGui frame
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	if (ImGuiManager::show_demo_window)
		ImGui::ShowDemoWindow(&ImGuiManager::show_demo_window);

	// 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
	{
		//static int counter = 0;
		//static char buffer[1024];
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

		//if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			//counter++;
		//ImGui::SameLine();
		//ImGui::Text("counter = %d", counter);

		// Buffer
		//ImGui::InputText("Butts", buffer, sizeof(buffer));

		// camera control
		m_Camera.CamerImGui();

		ImGui::End();
	}

	// Rendering
	ImGui::Render();

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

void GameCore::DrawMesh(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader)
{
	auto obj = model.GetObjCBuffer();
	objCBufferRef = TD3D12RHI::CreateConstantBuffer(&obj, sizeof(ObjCBuffer));
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
		shader.BindParameters();
		meshes[i].DrawMesh(g_CommandContext);
	}
}

void GameCore::LoadPipeline()
{
	uint32_t dxgiFactoryFlags = 0;
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

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
	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

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
	m_Camera.SetPosition(0, 2, -10);

	// load model
	ModelManager::LoadModel();
	boxMeshes.CreateBox(2, 2, 2, 3);

	// load Texture
	TextureManager::LoadTexture();

	g_CommandContext.FlushCommandQueue();
}

void GameCore::PopulateCommandList()
{
	// before reset a allocator, all commandlist associated with the allocator have completed
	g_CommandContext.FlushCommandQueue();
	// reset CommandAllocator and CommandList
	g_CommandContext.ResetCommandAllocator();
	g_CommandContext.ResetCommandList();

	// set necessary state
	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["pso"].GetRootSignature());
	g_CommandContext.GetCommandList()->RSSetViewports(1, &m_viewport);
	g_CommandContext.GetCommandList()->RSSetScissorRects(1, &m_scissorRect);

	// indicate that back buffer will be used as a render target
	g_CommandContext.Transition(m_renderTragetrs[m_frameIndex].GetD3D12Resource(), D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE);

	// indicate that back buffer will be used as a render target
	g_CommandContext.GetCommandList()->ClearRenderTargetView(m_renderTragetrs[m_frameIndex].GetRTV(), (FLOAT*)clearColor, 0, nullptr);
	g_CommandContext.GetCommandList()->ClearDepthStencilView(TD3D12RHI::g_DepthBuffer.GetDSV(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0, 0, 0, nullptr);

	g_CommandContext.GetCommandList()->OMSetRenderTargets(1, &m_renderTragetrs[m_frameIndex].GetRTV(), TRUE, &TD3D12RHI::g_DepthBuffer.GetDSV());

	// Record commands
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["pso"].GetPSO());

	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["nanosuit"], m_shaderMap["modelShader"]);
	DrawMesh(g_CommandContext, ModelManager::m_ModelMaps["wall"], m_shaderMap["modelShader"]);

	// sky box
	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(PSOManager::m_gfxPSOMap["skyboxPSO"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["skyboxPSO"].GetPSO());
	
	m_shaderMap["skyboxShader"].SetParameter("objCBuffer", objCBufferRef);
	m_shaderMap["skyboxShader"].SetParameter("passCBuffer", passCBufferRef);
	m_shaderMap["skyboxShader"].SetParameter("CubeMap", TextureManager::m_SrvMaps["skybox"]);
	m_shaderMap["skyboxShader"].SetDescriptorCache(boxMeshes.GetTD3D12DescriptorCache());
	m_shaderMap["skyboxShader"].BindParameters();
	boxMeshes.DrawMesh(g_CommandContext);

	// ImGui
	ID3D12DescriptorHeap* Heaps2[] = { g_ImGuiSrvHeap.Get() };
	g_CommandContext.GetCommandList()->SetDescriptorHeaps(1, Heaps2);
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_CommandContext.GetCommandList());

	// indicate that the back buffer will now be used to present
	g_CommandContext.Transition(m_renderTragetrs[m_frameIndex].GetD3D12Resource(), D3D12_RESOURCE_STATE_PRESENT);
	g_CommandContext.Transition(g_DepthBuffer.GetD3D12Resource(), D3D12_RESOURCE_STATE_COMMON);
}

void GameCore::WaitForPreviousFrame()
{
	g_CommandContext.FlushCommandQueue();
	
	DescriptorCache->Reset();

	m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
}
