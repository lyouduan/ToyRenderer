#include "D3D12DescriptorCache.h"
#include "DXSamplerHelper.h"

TD3D12DescriptorCache::TD3D12DescriptorCache(ID3D12Device* InDevice)
	: D3DDevice(InDevice)
{
	// Descriptor heap for cbv srv uav
	CreateCacheCbvSrvUavDescriptorHeap();

	// Descriptor heap for rtv
	CreateCacheRtvDescriptorHeap();
}

TD3D12DescriptorCache::~TD3D12DescriptorCache()
{
}

CD3DX12_GPU_DESCRIPTOR_HANDLE TD3D12DescriptorCache::AppendCbvSrvUavDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& SrcDescriptors)
{
	// append to heap
	uint32_t SlotsNeeded = (uint32_t)SrcDescriptors.size();
	assert(CbvSrvUavDescriptorOffset + SlotsNeeded < MaxCbvSrvUavDescriptorCount);

	auto CpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		CacheCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		CbvSrvUavDescriptorOffset,
		CbvSrvUavDescriptorSize);

	D3DDevice->CopyDescriptors(1, &CpuDescriptorHandle, &SlotsNeeded, SlotsNeeded,
		SrcDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// GetGpuDescriptorHandle
	auto GpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CacheCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// increase descriptor offset
	CbvSrvUavDescriptorOffset += SlotsNeeded;

	return GpuDescriptorHandle;
}

void TD3D12DescriptorCache::AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& OutGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& OutCpuHandle)
{
	// append to heap
	uint32_t SlotsNeeded = (uint32_t)RtvDescriptors.size();
	assert(RtvDescriptorOffset + SlotsNeeded < MaxRtvDescriptorCount);

	auto CpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		CacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		RtvDescriptorOffset,
		RtvDescriptorSize);

	D3DDevice->CopyDescriptors(1, &CpuDescriptorHandle, &SlotsNeeded, SlotsNeeded,
		RtvDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// GetCPU\GpuDescriptorHandle
	OutCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	OutGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// increase descriptor offset
	RtvDescriptorOffset += SlotsNeeded;
}

void TD3D12DescriptorCache::Reset()
{
	ResetCacheCbvSrvUavDescriptorHeap();
	ResetCacheRtvDescriptorHeap();
}

void TD3D12DescriptorCache::CreateCacheCbvSrvUavDescriptorHeap()
{
	// create the descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc = {};
	SrvHeapDesc.NumDescriptors = MaxCbvSrvUavDescriptorCount;
	SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&SrvHeapDesc, IID_PPV_ARGS(&CacheCbvSrvUavDescriptorHeap)));
	SetDebugName(CacheCbvSrvUavDescriptorHeap.Get(), L"TD3D12DescriptorCache CacheCbvSrvUavDescriptor");

	CbvSrvUavDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void TD3D12DescriptorCache::CreateCacheRtvDescriptorHeap()
{
	// create the descriptor heap
	D3D12_DESCRIPTOR_HEAP_DESC RtvHeapDesc = {};
	RtvHeapDesc.NumDescriptors = MaxRtvDescriptorCount;
	RtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	RtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&RtvHeapDesc, IID_PPV_ARGS(&CacheRtvDescriptorHeap)));
	SetDebugName(CacheRtvDescriptorHeap.Get(), L"TD3D12DescriptorCache CacheRtvDescriptor");

	RtvDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void TD3D12DescriptorCache::ResetCacheCbvSrvUavDescriptorHeap()
{
	CbvSrvUavDescriptorOffset = 0;
}

void TD3D12DescriptorCache::ResetCacheRtvDescriptorHeap()
{
	RtvDescriptorOffset = 0;
}
