#pragma once
#include "DirectXMath.h"

class TD3D12CommandContext;

namespace ImGuiManager
{
	extern bool show_demo_window;
	extern bool useCubeMap;
	extern bool bEnableDeferredRendering;
	
	extern DirectX::XMFLOAT3 lightPos;

	void InitImGui();

	void StartRenderImGui();

	void EndRenderImGui(TD3D12CommandContext& gfxContext);

	void DestroyImGui();
};

