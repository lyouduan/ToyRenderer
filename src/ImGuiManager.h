#pragma once
#include "DirectXMath.h"

class TD3D12CommandContext;

enum class GBufferType
{
	Position,
	Normal,
	Albedo,
	Specular,
	Count,
};

namespace ImGuiManager
{
	extern bool show_demo_window;
	extern bool useCubeMap;
	extern bool bEnableDeferredRendering;
	extern bool bEnableForwardPuls;
	extern bool bDebugGBuffers;
	extern int  GbufferType;
	
	extern DirectX::XMFLOAT3 lightPos;
	extern DirectX::XMFLOAT3 lightColor;
	extern float Intensity;
	extern DirectX::XMFLOAT3 modelPosition;

	extern float RotationY;
	extern float scale;

	extern float clearColor[4];

	void InitImGui();

	void StartRenderImGui();

	void EndRenderImGui(TD3D12CommandContext& gfxContext);

	void RenderAllItem();
	void RenderModelItem();
	void RenderLightItem();

	void RenderCombo();

	void DestroyImGui();
};

