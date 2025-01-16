#pragma once
#include "stdafx.h"
#include "DXSample.h"
#include "D3D12CommandContext.h"
#include "D3D12PixelBuffer.h"
#include "D3D12Buffer.h"
#include "Camera.h"
#include "ModelLoader.h"
#include "Mesh.h"
#include "Shader.h"
#include "SceneCaptureCube.h"
#include "PSO.h"
#include "Render.h"

using namespace DirectX;

using Microsoft::WRL::ComPtr;

class GameCore : public DXSample
{
public:
	GameCore(uint32_t width, uint32_t height, std::wstring name);
	void OnInit() override;
	void OnUpdate(const GameTimer& gt) override;
	void OnRender() override;
	void OnDestroy() override;

private:

	void UpdateImGui();

	void DrawMesh(TD3D12CommandContext& gfxContext, ModelLoader& model, TShader& shader);

	// pipleline objects
	CD3DX12_VIEWPORT m_viewport;;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;

	TD3D12ConstantBufferRef objCBufferRef;
	TD3D12ConstantBufferRef lightObjCBufferRef;
	TD3D12ConstantBufferRef passCBufferRef;
	TD3D12ConstantBufferRef matCBufferRef;
	MatCBuffer matCB;


	std::unique_ptr<TRender> m_Render;

	XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
	float scale = 1.0;

	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_LightMatrix;

	float totalTime = 0;
	float RotationY = 0.5;
	float clearColor[4] = { 0.9, 0.9, 0.9, 1.0 };

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
};
