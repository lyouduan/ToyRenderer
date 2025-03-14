#include "ImGuiManager.h"
#include "D3D12RHI.h"
#include "Win32Application.h"
#include "RenderInfo.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using namespace TD3D12RHI;

namespace ImGuiManager
{
	bool show_demo_window = false;
	bool useCubeMap = false;
	bool bEnableDeferredRendering = false;
	bool bEnableForwardPuls = false;
	bool bEnableShadowMap = false;
	bool bEnableTAA = false;
	bool bEnableFXAA = false;
	bool bEnableCSMInst = false;
	bool bDebugGBuffers = false;
	int  GbufferType = 0;
	int  ShadowType = (int)ShadowType::Count;
	
	SSAOParams ssaoParams = { 0.03f , 0.01f, 0.03f, 0.001f };

	float  cascadeBlendColor = 0.0f;

	DirectX::XMFLOAT3 lightPos = { 0.0, 10.0, -5.0 };
	DirectX::XMFLOAT3 lightColor = { 1.0, 1.0, 1.0 };
	float Intensity = 100;
	
	MatCBuffer matCB;

	DirectX::XMFLOAT3 modelPosition = { 0.0f, 5.0f, 0.0f };

	float RotationY = 0.5;
	float scale = 1.0;

	float clearColor[4] = { 0.9, 0.9, 0.9, 1.0 };

	void InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(Win32Application::GetHwnd());

		// Setup Platform/Renderer backends
		//ImGui_ImplDX12_InitInfo init_info = {};
		//init_info.Device = g_Device;
		//init_info.CommandQueue = g_CommandContext.GetCommandQueue();
		//init_info.NumFramesInFlight = FrameCount;
		//init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM; // Or your render target format.
		//init_info.SrvDescriptorHeap = ImGuiSrvHeap;

		auto cpuhandle = g_ImGuiSrvHeap->GetCPUDescriptorHandleForHeapStart();
		auto gpuhandle = g_ImGuiSrvHeap->GetGPUDescriptorHandleForHeapStart();

		ImGui_ImplDX12_Init(g_Device, FrameCount, DXGI_FORMAT_R8G8B8A8_UNORM, g_ImGuiSrvHeap.Get(), cpuhandle, gpuhandle);
	
	}

	void StartRenderImGui()
	{
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		if (ImGuiManager::show_demo_window)
			ImGui::ShowDemoWindow(&ImGuiManager::show_demo_window);
	}

	void EndRenderImGui(TD3D12CommandContext& gfxContext)
	{
		ID3D12DescriptorHeap* Heaps2[] = { g_ImGuiSrvHeap.Get() };
		gfxContext.GetCommandList()->SetDescriptorHeaps(1, Heaps2);
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), gfxContext.GetCommandList());
	}

	void RenderAllItem()
	{
		RenderModelItem();

		RenderLightItem();

		RenderPBRItem();

		RenderSSAOItem();
	}

	void RenderPBRItem()
	{
		ImGui::Text("Model PBR");
		ImGui::ColorEdit4("baseColor", (float*)&matCB.DiffuseAlbedo);
		ImGui::SliderFloat("Roughness", &matCB.Roughness, 0.001f, 1.0f);
		ImGui::SliderFloat("Metallic", &matCB.metallic, 0.0f, 1.0f);
	}

	void RenderModelItem()
	{
		ImGui::Begin("ImGui!");                          // Create a window called "ImGui!" and append into it.
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		// Display some text (you can use a format strings too)
		ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our window open/close state
		//ImGui::Checkbox("Another Window", &show_another_window);
		ImGui::Text("Model Control Parameters");
		ImGui::SliderFloat("RotationY", &RotationY, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Scale", &scale, 0.0f, 10.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Model X", &modelPosition.x, -20.0f, 20.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Model Y", &modelPosition.y, -20.0f, 20.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::SliderFloat("Model Z", &modelPosition.z, -20.0f, 20.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
	}

	void RenderLightItem()
	{
		//ImGui::ColorEdit3("clear color", (float*)&clearColor); // Edit 3 floats representing a color
		ImGui::NewLine();
		ImGui::Text("Light Info");
		ImGui::SliderFloat("light intensity", (float*)&Intensity, 0, 1000); // Edit 3 floats representing a color
		ImGui::SliderFloat3("light Position", (float*)&lightPos, -300, 300);
		ImGui::ColorEdit3("light color", (float*)&lightColor); // Edit 3 floats representing a color
	}

	void RenderSSAOItem()
	{
		ImGui::NewLine();
		ImGui::Text("SSAO Parameters");
		ImGui::SliderFloat("OcclusionFadeEnd", &ssaoParams.OcclusionFadeEnd, 0.01, 1.0f);
		ImGui::SliderFloat("OcclusionFadeStart", &ssaoParams.OcclusionFadeStart, 0.01, 1.0f);
		ImGui::SliderFloat("OcclusionRadius", &ssaoParams.OcclusionRadius, 0.01, 1.0f);
		ImGui::SliderFloat("SurfaceEpsilon", &ssaoParams.SurfaceEpsilon, 0.01, 1.0f);
	}

	void RenderCombo()
	{	/*
		enum class GBufferType
		{
			Position,
			Normal,
			Albedo,
			Specular,
			Count,
		};*/
		const char* items[] = { "Position", "Normal", "Albedo", "Specular","SSAO", "Velocity", "LightMap"};

		// 创建选择框
		if (ImGui::BeginCombo("Select an option", items[GbufferType])) {
			// 遍历选项
			for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
				bool is_selected = (GbufferType == i); // 判断是否为当前选中项
				if (ImGui::Selectable(items[i], is_selected)) {
					GbufferType = i; // 更新当前选项
				}
				
			}
			ImGui::EndCombo();
		}

	}

	void ShadowTypeCombo()
	{	/*
		enum class GBufferType
		{
			Position,
			Normal,
			Albedo,
			Specular,
			Count,
		};*/
		const char* items[] = { "NONE", "PCSS", "VSM", "VSSM", "ESM", "EVSM","CSM", "PCF"};

		// 创建选择框
		if (ImGui::BeginCombo("Select an option", items[ShadowType])) {
			// 遍历选项
			for (int i = 0; i < IM_ARRAYSIZE(items); i++) {
				bool is_selected = (ShadowType == i); // 判断是否为当前选中项
				if (ImGui::Selectable(items[i], is_selected)) {
					ShadowType = i; // 更新当前选项
				}
			}
			ImGui::EndCombo();
		}


	}

	void DestroyImGui()
	{
		ImGui_ImplDX12_Shutdown();
		
		ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

}

