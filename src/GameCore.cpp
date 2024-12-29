#include "GameCore.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12RHI.h"
#include "D3D12Texture.h"
#include "Display.h"
#include <chrono>
#include "ImGuiManager.h"
#include "PSOManager.h"

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
	XMMATRIX scalingMat = XMMatrixScaling(scale, scale, scale);

	//totalTime += gt.DeltaTime() * 0.01;
	float rotate_angle = static_cast<float>(RotationY * 360 /* * totalTime*/);
	const XMVECTOR rotationAxis = XMVectorSet(0, 1, 0, 0);
	XMMATRIX rotationYMat = XMMatrixRotationAxis(rotationAxis, XMConvertToRadians(rotate_angle));
	XMMATRIX translationMatrix = XMMatrixTranslation(position.x, position.y, position.z);

	m_ModelMatrix = scalingMat * rotationYMat * translationMatrix;

	// Update the view matrix.
	const XMVECTOR eyePosition = XMVectorSet(0, 0, -50, 1);
	const XMVECTOR focusPoint = XMVectorSet(0, 0, 0, 1);
	const XMVECTOR upDirection = XMVectorSet(0, 1, 0, 0);

	m_Camera.SetLookAt(eyePosition, focusPoint, upDirection);

	//m_ViewMatrix = XMMatrixLookAtLH(eyePosition, focusPoint, upDirection);

	// Update the projection matrix.
	float aspectRatio = GetWidth() / static_cast<float>(GetHeight());
	m_Camera.SetAspectRatio(aspectRatio);
	//m_ProjectionMatrix = XMMatrixPerspectiveFovLH(XMConvertToRadians(45.0), aspectRatio, 0.1f, 100.0f);
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

	model.Close();
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
	{
		PSOManager::InitializePSO();
	}

	// load model
	{
		if (!model.Load("./models/nanosuit/nanosuit.obj"))
			assert(false);

		boxMeshes.CreateBox(2, 2, 2, 3);
	}

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
	//g_CommandContext.GetCommandList()->SetGraphicsRootSignature(m_shader->RootSignature.Get());
	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(&PSOManager::m_gfxPSOMap["pso"].GetRootSignature());
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

	//g_CommandContext.GetCommandList()->SetPipelineState(m_pipelineState.Get());

	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["pso"].GetPSO());

	// Update the MVP matrix
	//XMMATRIX mvpMatrix = XMMatrixMultiply(m_ModelMatrix, m_Camera.GetViewProjMat());

	//g_CommandContext.GetCommandList()->SetGraphicsRoot32BitConstants(0, sizeof(XMMATRIX) / 4, &mvpMatrix, 0);
	PasscBuffer passCB;
	passCB.Model = m_ModelMatrix;
	passCB.VP = m_Camera.GetViewProjMat();

	cBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PasscBuffer));

	//model.Draw(g_CommandContext, m_shader.get(), cBufferRef);

	auto meshes = model.GetMeshes();
	for (auto& mesh : meshes)
	{
		m_shaderMap["modelShader"].SetParameter("MVPcBuffer", cBufferRef);
		// draw call
		auto m_SRV = mesh.GetSRV();
		m_shaderMap["modelShader"].SetParameter("diffuseMap", m_SRV[0]);
		//m_shader->SetParameter("specularMap", m_SRV[1]);
		//m_shader->SetParameter("normalMap", m_SRV[2]);

		m_shaderMap["modelShader"].SetDescriptorCache(mesh.GetTD3D12DescriptorCache());
		m_shaderMap["modelShader"].BindParameters();
		mesh.DrawMesh(g_CommandContext);
	}


	g_CommandContext.GetCommandList()->SetGraphicsRootSignature(&PSOManager::m_gfxPSOMap["boxPSO"].GetRootSignature());
	g_CommandContext.GetCommandList()->SetPipelineState(PSOManager::m_gfxPSOMap["boxPSO"].GetPSO());
	XMMATRIX translationMatrix = XMMatrixTranslation(position.x + 10, position.y-10, position.z);
	passCB.Model = translationMatrix * m_ModelMatrix;
	cBufferRef = TD3D12RHI::CreateConstantBuffer(&passCB, sizeof(PasscBuffer));
	m_shaderMap["boxShader"].SetParameter("MVPcBuffer", cBufferRef);
	m_shaderMap["boxShader"].SetDescriptorCache(boxMeshes.GetTD3D12DescriptorCache());
	m_shaderMap["boxShader"].BindParameters();
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
