#pragma once
#include "DXSample.h"
#include "D3D12CommandContext.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class D3D12HelloTriangle : public DXSample
{
public:
	D3D12HelloTriangle(uint32_t width, uint32_t height, std::wstring name);
	void OnInit() override;
	void OnUpdate() override;
	void OnRender() override;
	void OnDestroy() override;

private:
	static const uint32_t FrameCount = 2;

	struct Vertex {
		XMFLOAT3 position;
		XMFLOAT4 color;
	};

	// pipleline objects
	CD3DX12_VIEWPORT m_viewport;;
	CD3DX12_RECT m_scissorRect;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12Device> m_device;
	ComPtr<ID3D12Resource> m_renderTragetrs[FrameCount];
	ComPtr<ID3D12Resource> m_Depth;

	//ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	//ComPtr<ID3D12CommandQueue> m_commandQueue;
	//ComPtr<ID3D12GraphicsCommandList> m_commandList;

	ComPtr<ID3D12RootSignature> m_rootSignature;
	ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
	ComPtr<ID3D12PipelineState> m_pipelineState;
	uint32_t m_rtvDescriptorSize;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> RtvDescriptors;
	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> DsvDescriptors;

	// app resource
	ComPtr<ID3D12Resource> m_vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW m_indexBufferView;
	// synchronization objects
	uint32_t m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValue;

	DirectX::XMMATRIX m_ModelMatrix;
	DirectX::XMMATRIX m_ViewMatrix;
	DirectX::XMMATRIX m_ProjectionMatrix;

	void LoadPipeline();
	void LoadAssets();
	void PopulateCommandList();
	void WaitForPreviousFrame();
};
