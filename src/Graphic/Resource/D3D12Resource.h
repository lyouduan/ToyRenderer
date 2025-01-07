#pragma once
#include "stdafx.h"

class TD3D12BuddyAllocator;

class TD3D12Resource
{
public:
	TD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON);

	TD3D12Resource(ID3D12Resource* InD3DResource, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON);

	// for upload buffer
	void Map();

	void Destroy();

public:

	Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	D3D12_RESOURCE_STATES CurrentState;

	// for upload buffer
	void* MappedBaseAddress = nullptr;
};


// buddy info
struct TD3D12BuddyBlockData
{
	uint32_t Offset = 0;
	uint32_t Order = 0;
	uint32_t ActualUsedSize = 0;

	// Resource in Block
	TD3D12Resource* PlacedResource = nullptr;
};

class TD3D12ResourceLocation
{
public:

	enum class EResourceLocationType
	{
		Undefined,
		StandAlone,
		SubAllocation,
	};

public:

	TD3D12ResourceLocation();

	~TD3D12ResourceLocation();

	void ReleaseResource();

	void SetType(EResourceLocationType Type) {}

public:

	EResourceLocationType ResourceLocationType = EResourceLocationType::Undefined;

	// SubAllocation
	TD3D12BuddyAllocator* Allocator = nullptr;

	TD3D12BuddyBlockData BlockData;

	// StandAlone resource
	TD3D12Resource* UnderlyingResource = nullptr;

	union
	{
		uint64_t OffsetFromBaseOfResource; // ManualSubAllocation, committed Resource
		uint64_t OffsetFromBaseOfHeap; // PlacedResource
	};

	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	// mapping for upload buffer
	void* MappedAddress = nullptr;
};

template<typename T>
class TD3D12ScopeMap
{
public:
	TD3D12ScopeMap(TD3D12Resource* Resource)
	{
		D3DResource = Resource->D3DResource.Get();
		D3DResource->Map(0, nullptr, reinterpret_cast<void**>(&MappedData));
	}

	~TD3D12ScopeMap()
	{
		D3DResource->Unmap(0, nullptr);
	}

	T* GetMappedData() { return MappedData; }

private:
	ID3D12Resource* D3DResource = nullptr;

	T* MappedData = nullptr;
};




