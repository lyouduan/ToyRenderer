#pragma once
#include "stdafx.h"
#include "D3D12DescriptorCache.h"
#include "D3D12Resource.h"

class TD3D12CommandContext
{
public:
	TD3D12CommandContext();

	~TD3D12CommandContext();

	void CreateCommandContext(ID3D12Device* InDevice);

	void DestroyCommandContext();

	ID3D12CommandQueue* GetCommandQueue() { return CommandQueue.Get(); }

	ID3D12GraphicsCommandList* GetCommandList() { return CommandList.Get(); }

	TD3D12DescriptorCache* GetDescriptorCache() { return DescriptorCache.get(); }

	void Transition(TD3D12Resource* resource, D3D12_RESOURCE_STATES afterState);

	void ResetCommandAllocator();

	void ResetCommandList();

	void ExecuteCommandLists();

	void FlushCommandQueue();

	void EndFrame();

private:
	
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> CommandQueue = nullptr;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CommandListAlloc = nullptr;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CommandList = nullptr;

	std::unique_ptr<TD3D12DescriptorCache> DescriptorCache = nullptr;

private:
	Microsoft::WRL::ComPtr<ID3D12Fence> Fence = nullptr;

	UINT64 CurrentFenceValue = 0;
};

