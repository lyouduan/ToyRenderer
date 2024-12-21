#pragma once
#include "stdafx.h"
#include "DXSample.h"
#include "D3D12CommandContext.h"
#include "D3D12PixelBuffer.h"
#include "D3D12Buffer.h"
#include "Camera.h"
#include "ModelLoader.h"

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

	struct Vertex {
		XMFLOAT3 position;
		XMFLOAT2 tex;
	};

	// pipleline objects
	CD3DX12_VIEWPORT m_viewport;;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;

	//ComPtr<ID3D12Resource> m_renderTragetrs[FrameCount];
	D3D12ColorBuffer m_renderTragetrs[FrameCount];

	TD3D12IndexBufferRef indexBufferRef;
	TD3D12VertexBufferRef vertexBufferRef;
	ModelLoader model;
	//ComPtr<ID3D12Resource> m_Depth;

	//ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	//ComPtr<ID3D12CommandQueue> m_commandQueue;
	//ComPtr<ID3D12GraphicsCommandList> m_commandList;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	//ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	//uint32_t m_rtvDescriptorSize;

	//std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DsvDescriptors;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_SRV;

	std::shared_ptr<TD3D12DescriptorCache> descriptorCache = nullptr;

	// app resource
	//ComPtr<ID3D12Resource> m_vertexBuffer;
	//D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	//D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

	// synchronization objects
	uint32_t m_frameIndex;
	//HANDLE m_fenceEvent;
	//ComPtr<ID3D12Fence> m_fence;
	//uint64_t m_fenceValue;

	Camera m_Camera;

	DirectX::XMMATRIX m_ModelMatrix;
	//DirectX::XMMATRIX m_ViewMatrix;
	//DirectX::XMMATRIX m_ProjectionMatrix;

	float totalTime = 0;
	float speed = 0.1;
	float clearColor[4] = {0.9, 0.9, 0.9, 1.0};

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
};
