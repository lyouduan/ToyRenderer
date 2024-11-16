#include "D3D12DescriptorCache.h"
#include "DXSamplerHelper.h"

TD3D12DescriptorCache::TD3D12DescriptorCache(ID3D12Device* InDevice)
	: D3DDevice(InDevice)
{
	// create cache for Descriptor Heap
	CreateCacheCbvSrvUavDescriptorHeap();
	CreateCacheRtvDescriptorHeap();
}

TD3D12DescriptorCache::~TD3D12DescriptorCache()
{
}

CD3DX12_GPU_DESCRIPTOR_HANDLE TD3D12DescriptorCache::AppendCbvSrvUavDescriptors(const std::vector<D3D12_GPU_DESCRIPTOR_HANDLE>& SrvDescriptors)
{
	// Append to heap
	
	// caclculate the size of requested Descriptor heaps
	uint32_t SlotsNeeded = (uint32_t)SrvDescriptors.size();
	assert(CbvSrvUavDescriptorOffset + SlotsNeeded < MaxCbvSrvUavDescriptorCount);

	// 计算当前空闲堆的句柄
	auto CpuDesciptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheCbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), CbvSrvUavDescriptorOffset, CbvSrvUavDescriptorSize);

	// Get GpuDescriptorHandle
	auto GpuDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CacheCbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), CbvSrvUavDescriptorOffset, CbvSrvUavDescriptorSize);

	// Increase descriptor offset
	// 计算分配后的空闲堆偏移
	CbvSrvUavDescriptorOffset += SlotsNeeded;

	return GpuDescriptorHandle;
}

void TD3D12DescriptorCache::AppendRtvDescriptors(const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& RtvDescriptors, CD3DX12_GPU_DESCRIPTOR_HANDLE& OutGpuHandle, CD3DX12_CPU_DESCRIPTOR_HANDLE& OutCpuHandle)
{
	// append to heap
	uint32_t SlotsNeeded = RtvDescriptors.size();
	assert(RtvDescriptorOffset + SlotsNeeded < MaxRtvDescriptorCount);

	auto CpuDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), RtvDescriptorOffset, RtvDescriptorSize);
	D3DDevice->CopyDescriptors(1, &CpuDescriptorHandle, &SlotsNeeded, SlotsNeeded, RtvDescriptors.data(), nullptr, D3D12_DESCRIPTOR_HEAP_TYPE_RTV); // 为什么在CopyDescriptor时，RtvDescriptor是D3D12_CPU_DESCRIPTOR_HANDLE？不能GPU，
	
	OutGpuHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), RtvDescriptorOffset, RtvDescriptorSize);

	OutCpuHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(CacheRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), RtvDescriptorOffset, RtvDescriptorSize);

	// Increaset descriptor offset
	RtvDescriptorOffset += SlotsNeeded;
}

void TD3D12DescriptorCache::Reset()
{
	ResetCacheCbvSrvUavDescriptorHeap();
	ResetCacheRtvDescriptorHeap();
}

void TD3D12DescriptorCache::CreateCacheCbvSrvUavDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
	Desc.NumDescriptors = MaxCbvSrvUavDescriptorCount;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; // shader-visible
	Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; 

	ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&CacheCbvSrvUavDescriptorHeap)));
	SetDebugName(CacheCbvSrvUavDescriptorHeap.Get(), L"TD3D12DescriptorCache CacheCbvSrvUavDescriptorHeap");

	CbvSrvUavDescriptorSize = D3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void TD3D12DescriptorCache::CreateCacheRtvDescriptorHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
	Desc.NumDescriptors = MaxRtvDescriptorCount;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&CacheRtvDescriptorHeap)));
	SetDebugName(CacheRtvDescriptorHeap.Get(), L"TD3D12DescriptorCache CacheCbvSrvUavDescriptorHeap");

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
