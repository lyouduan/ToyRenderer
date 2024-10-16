#include "DXSamplerHelper.h"

// 向前声明
class TD3D12BuddyAllocator;

// 资源类基类
class TD3D12Resource
{
public:
	TD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> InD3DResource, D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COMMON);

	void Map();

public:

	Microsoft::WRL::ComPtr<ID3D12Resource> D3DResource = nullptr;

	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	D3D12_RESOURCE_STATES CurrentState;

	// For upload buffer
	void* MappedBaseAddress = nullptr;
};

struct TD3D12BuddyBlockData
{
	uint32_t Offset = 0; 
	uint32_t Order = 0;
	uint32_t ActualUsedSize = 0;

	TD3D12Resource* PlacedResource = nullptr;
};

// 资源信息块：记录资源相关信息，底层资源堆，堆偏移量，堆划分方法
class TD3D12ResourceLocation
{
public:
	enum class EResourceLocationType
	{
		Undefined,
		StandAlone, // for createCommittedResource
		SubAlloction, // for createPlacedResource
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

	// Offset for two Types
	union 
	{
		uint32_t OffsetFromBaseOfResource;
		uint32_t OffsetFromBaseOfHeap;
	};

	D3D12_GPU_VIRTUAL_ADDRESS GPUVirtualAddress = 0;

	// About mapping, for upload buffer
	void* MappedAddress = nullptr;
};