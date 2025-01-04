#pragma once
class TD3D12CommandContext;

namespace ImGuiManager
{
	extern bool show_demo_window;
	extern bool useCubeMap;

	void InitImGui();

	void StartRenderImGui();

	void EndRenderImGui(TD3D12CommandContext& gfxContext);

	void DestroyImGui();
};

