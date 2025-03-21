#pragma once
#include "DirectXMath.h"
#include "RenderInfo.h"

class TD3D12CommandContext;

struct SSAOParams
{
	float    OcclusionRadius;
	float    OcclusionFadeStart;
	float    OcclusionFadeEnd;
	float    SurfaceEpsilon;
};

namespace ImGuiManager
{
	extern bool show_demo_window;
	extern bool useCubeMap;
	extern bool bEnableDeferredRendering;
	extern bool bEnableForwardPuls;
	extern bool bEnableShadowMap;
	extern bool bEnableTAA;
	extern bool bEnableFXAA;
	extern bool bEnableCSMInst;
	extern bool bDebugGBuffers;
	extern int  GbufferType;
	extern int  ShadowType;
	extern int  SceneType;

	extern float  cascadeBlendColor;

	extern MatCBuffer matCB;

	extern SSAOParams ssaoParams;
	
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
	void RenderPBRItem();
	void RenderSSAOItem();

	void RenderCombo();
	void ShadowTypeCombo();

	void DestroyImGui();
};

