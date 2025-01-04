#include "ImGuiManager.h"
#include "D3D12RHI.h"
#include "Win32Application.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

using namespace TD3D12RHI;

namespace ImGuiManager
{
	bool show_demo_window = false;
	bool useCubeMap = false;

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

	void DestroyImGui()
	{
		ImGui_ImplDX12_Shutdown();
		
		ImGui_ImplWin32_Shutdown();

		ImGui::DestroyContext();
	}

}

