#pragma once
#include "stdafx.h"

class TD3D12DescriptorCache
{
public:
	TD3D12DescriptorCache(ID3D12Device* InDevice);

	~TD3D12DescriptorCache();

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCacherCbvSrvUavDescriptorHeap() {
		return CacheCbvSrvUavDescriptorHeap;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE AppendCbvSrvUavDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& SrcDescriptors);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetCacheRtvDescriptorHeap() { return CacheRtvDescriptorHeap; }

	void AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& OutGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& OutCpuHandle);

	void Reset();

	UINT GetRtvDescriptorSize() { return RtvDescriptorSize; }
	UINT GetCbvSrvUavDescriptorSize() { return CbvSrvUavDescriptorSize; }

private:
	void CreateCacheCbvSrvUavDescriptorHeap();

	void CreateCacheRtvDescriptorHeap();

	void ResetCacheCbvSrvUavDescriptorHeap();

	void ResetCacheRtvDescriptorHeap();

private:

	ID3D12Device* D3DDevice = nullptr;

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CacheCbvSrvUavDescriptorHeap = nullptr;

	UINT CbvSrvUavDescriptorSize;

	static const int MaxCbvSrvUavDescriptorCount = 2048;

	uint32_t CbvSrvUavDescriptorOffset = 0;

private:

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CacheRtvDescriptorHeap = nullptr;

	UINT RtvDescriptorSize;

	static const int MaxRtvDescriptorCount = 1024;

	uint32_t RtvDescriptorOffset = 0;
};

