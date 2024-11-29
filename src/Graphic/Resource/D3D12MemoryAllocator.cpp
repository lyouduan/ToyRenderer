#include "D3D12MemoryAllocator.h"
#include "DXSamplerHelper.h"

TD3D12BuddyAllocator::TD3D12BuddyAllocator(ID3D12Device* InDevice, const TAllocatorInitData& InInitData)
	: D3DDevice(InDevice), InitData(InInitData)
{
	Initialize();
}

TD3D12BuddyAllocator::~TD3D12BuddyAllocator()
{
	if (BackingResource)
	{
		delete BackingResource;
	}

	if (BackingHeap)
	{
		BackingHeap->Release();
	}
}

bool TD3D12BuddyAllocator::AllocResource(uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
	uint32_t SizeToAllocate = GetSizeToAllocate(Size, Alignment);

	if (CanAllocate(SizeToAllocate))
	{
		// Allocate Block
		const uint32_t UnitSize = SizeToUnitSize(SizeToAllocate);
		const uint32_t Order = UnitSizeToOrder(UnitSize);
		const uint32_t Offset = AllocateBlock(Order); // This is  the offset in MinBlockSize units
		const uint32_t AllocSize = UnitSize * MinBlockSize;
		TotalAllocSize += AllocSize;

		// calculate AlignedOffsetFromResourceBase
		const uint32_t OffsetFromBaseOfResource = GetAllocOffsetInBytes(Offset);
		uint32_t AlignedOffsetFromResourceBase = OffsetFromBaseOfResource;

		if (Alignment != 0 && OffsetFromBaseOfResource % Alignment != 0)
		{
			AlignedOffsetFromResourceBase = AlignArbitrary(OffsetFromBaseOfResource, Alignment);

			uint32_t Padding = AlignedOffsetFromResourceBase - OffsetFromBaseOfResource;
			assert((Padding + Size) <= AllocSize);
		}

		assert((AlignedOffsetFromResourceBase % Alignment) == 0);

		// save allocation info to ResourceLocation
		ResourceLocation.SetType(TD3D12ResourceLocation::EResourceLocationType::SubAllocation);
		ResourceLocation.BlockData.Order = Order;
		ResourceLocation.BlockData.Offset = Offset;
		ResourceLocation.BlockData.ActualUsedSize = Size;
		ResourceLocation.Allocator = this;

		if (InitData.AllocatioStrategy == EAllocationStrategy::ManualSubAllocation)
		{
			// 手动划分，计算GPUVirtualAddress for default buffer
			ResourceLocation.UnderlyingResource = BackingResource;
			ResourceLocation.OffsetFromBaseOfResource = AlignedOffsetFromResourceBase;
			ResourceLocation.GPUVirtualAddress = BackingResource->GPUVirtualAddress + AlignedOffsetFromResourceBase;

			// 计算MappedAddress for upload buffer
			if (InitData.HeapType == D3D12_HEAP_TYPE_UPLOAD)
			{
				ResourceLocation.MappedAddress = ((uint8_t*)BackingResource->MappedBaseAddress + AlignedOffsetFromResourceBase);
			}
		}
		else
		{
			ResourceLocation.OffsetFromBaseOfHeap = AlignedOffsetFromResourceBase;

			// Place Resource are initialized by caller
		}

		return true;
	}
	else
	{
		return false;
	}
}

void TD3D12BuddyAllocator::Deallocate(TD3D12ResourceLocation& ResourceLocation)
{
	DeferredDeletionQueue.push_back(ResourceLocation.BlockData);
}

void TD3D12BuddyAllocator::CleanUpAllocations()
{
	for (int32_t i = 0; i < DeferredDeletionQueue.size(); ++i)
	{
		const TD3D12BuddyBlockData& Block = DeferredDeletionQueue[i];

		DeallocateInternal(Block);
	}

	// clear out all of the released blocks, don't allow the array to shrink
	DeferredDeletionQueue.clear();
}

void TD3D12BuddyAllocator::Initialize()
{
	// create backingHeap or backingResource

	// placed Resource : 创建一个堆，再分配给不同状态的资源
	if (InitData.AllocatioStrategy == EAllocationStrategy::PlacedResource)
	{
		// 定义堆类型，Default heap or Upload heap
		CD3DX12_HEAP_PROPERTIES HeapProperties(InitData.HeapType);
		D3D12_HEAP_DESC Desc = {};
		Desc.SizeInBytes = DEFAULT_POOL_SIZE;
		Desc.Properties = HeapProperties;
		Desc.Alignment = 0;
		Desc.Flags = InitData.HeapFlags;

		// create BackingHeap, we will create place resources on it
		ID3D12Heap* Heap = nullptr;
		ThrowIfFailed(D3DDevice->CreateHeap(&Desc, IID_PPV_ARGS(&Heap)));
		Heap->SetName(L"TD3D12BuddyAllocator BackingHeap");

		// DX12基于Com接口，引用计数机制，所以脱离了作用域Heap也不会析构
		BackingHeap = Heap;
	}
	else // ManualSubAllocation
	{
		// 创建committed Resource ： 一个Resource资源，逻辑上手动划分不同区域，分配给多个状态相同的资源
		CD3DX12_HEAP_PROPERTIES HeapProperties(InitData.HeapType);
		D3D12_RESOURCE_STATES  HeapResourceState;
		if (InitData.HeapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			HeapResourceState = D3D12_RESOURCE_STATE_GENERIC_READ; // for upload heap, it will never change the state
		}
		else // D3D12_HEAP_TYPE_DEFAULT
		{
			HeapResourceState = D3D12_RESOURCE_STATE_COMMON;
		}

		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DEFAULT_POOL_SIZE, InitData.ResourceFlags);

		// create committed resource, we will allocate sub regions on it
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		ThrowIfFailed(D3DDevice->CreateCommittedResource(
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&BufferDesc,
			HeapResourceState,
			nullptr,
			IID_PPV_ARGS(&Resource)));

		Resource->SetName(L"TD3D12BuddyAllocator BackingResource");

		// 将创建的Resource对象封装管理
		BackingResource = new TD3D12Resource(Resource);

		// mapping for upload buffer
		if (InitData.HeapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			BackingResource->Map();
		}
	}

	// Initialize free blocks, add the free block for MarOrder
	MaxOrder = UnitSizeToOrder(SizeToUnitSize(DEFAULT_POOL_SIZE));

	for (uint32_t i = 0; i <= MaxOrder; ++i)
	{
		FreeBlocks.emplace_back(std::set<uint32_t>());
	}

	// 起始为0，最高阶数
	FreeBlocks[MaxOrder].insert((uint32_t)0);
}

uint32_t TD3D12BuddyAllocator::AllocateBlock(uint32_t Order)
{
	uint32_t Offset;

	if (Order > MaxOrder)
	{
		assert(false);
	}

	if (FreeBlocks[Order].size() == 0) 
	{
		// 对应阶数无自由块，往高一阶寻找，并拆分
		uint32_t Left = AllocateBlock(Order + 1);

		uint32_t UnitSize = OrderToUnitSize(Order);

		uint32_t Right = Left + UnitSize;

		FreeBlocks[Order].insert(Right); // add the right block to the free pool]

		Offset = Left;
	}
	else // 有对应阶数自由块，则找最左侧的一块进行分配（set已经自动完成排序，底层为排序二叉树，红黑树)
	{
		auto It = FreeBlocks[Order].cbegin();
		Offset = *It;

		// Remove the block from the free list
		FreeBlocks[Order].erase(*It);
	}

	return Offset;
}

uint32_t TD3D12BuddyAllocator::GetSizeToAllocate(uint32_t Size, uint32_t Alignment)
{
	uint32_t SizeToAllocate = Size;
	
	// if the alignment doesn't match the block size
	if (Alignment != 0 && MinBlockSize % Alignment != 0)
	{
		SizeToAllocate = Size + Alignment;
	}

	return SizeToAllocate;
}

bool TD3D12BuddyAllocator::CanAllocate(uint32_t SizeToAllocate)
{
	// 已分配容量 等于 默认容量
	if (TotalAllocSize == DEFAULT_POOL_SIZE)
	{
		return false;
	}

	uint32_t BlockSize = DEFAULT_POOL_SIZE;

	// 从最高阶遍历 自由块进行分配
	for (int i = (int)FreeBlocks.size() - 1; i >= 0; i--)
	{
		if (FreeBlocks[i].size() && BlockSize >= SizeToAllocate)
			return true;

		// halve the block size
		BlockSize = BlockSize >> 1;

		//最小块容量 不能满足 请求容量
		if (BlockSize < SizeToAllocate) return false;
	}

	return false;
}

void TD3D12BuddyAllocator::DeallocateInternal(const TD3D12BuddyBlockData& Block)
{
	// 删除合并
	DeallocateBlock(Block.Offset, Block.Order);

	// 重新计算总分配容量
	uint32_t Size = OrderToUnitSize(Block.Order) * MinBlockSize;

	TotalAllocSize -= Size;

	// 删除指针
	if (InitData.AllocatioStrategy == EAllocationStrategy::PlacedResource)
	{
		// Release place resource
		assert(Block.PlacedResource != nullptr);

		delete Block.PlacedResource;
	}
}

void TD3D12BuddyAllocator::DeallocateBlock(uint32_t Offset, uint32_t Order)
{
	// Get buddy block
	uint32_t Size = OrderToUnitSize(Order);
	// 计算出伙伴块的偏移
	uint32_t Buddy = GetBuddyOffset(Offset, Size);

	auto It = FreeBlocks[Order].find(Buddy);
	if (It != FreeBlocks[Order].end())// If buddy block is free, merge it
	{
		// Deallocate merged blocks
		DeallocateBlock(min(Offset, Buddy), Order + 1);

		// Remove the buddy from the free list
		FreeBlocks[Order].erase(*It);
	}
	else
	{
		// add the block to the free list
		FreeBlocks[Order].insert(Offset);
	}
}

TD3D12MultiBuddyAllocator::TD3D12MultiBuddyAllocator(ID3D12Device* InDevice, const TD3D12BuddyAllocator::TAllocatorInitData& InInitData)
	: Device(InDevice), InitData(InInitData)
{
}

TD3D12MultiBuddyAllocator::~TD3D12MultiBuddyAllocator()
{
}

bool TD3D12MultiBuddyAllocator::AllocResource(uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
	for (auto& Allocator : Allocators) // try to use existing allocators
	{
		if (Allocator->AllocResource(Size, Alignment, ResourceLocation))
		{
			return true;
		}
	}

	// create new allocator
	auto Allocator = std::make_shared<TD3D12BuddyAllocator>(Device, InitData);
	Allocators.push_back(Allocator);

	bool Result = Allocator->AllocResource(Size, Alignment, ResourceLocation);
	assert(Result);

	return true;
}

void TD3D12MultiBuddyAllocator::CleanUpAllocations()
{
	for (auto& Allocator : Allocators)
		Allocator->CleanUpAllocations();
}

TD3D12UploadBufferAllocator::TD3D12UploadBufferAllocator(ID3D12Device* InDevice)
{
	TD3D12BuddyAllocator::TAllocatorInitData InitData;
	InitData.AllocatioStrategy = TD3D12BuddyAllocator::EAllocationStrategy::ManualSubAllocation; // UploadBuffer状态一致，可以共用一个Resource，逻辑上进行不同区域的划分
	InitData.HeapType = D3D12_HEAP_TYPE_UPLOAD;
	InitData.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;

	Allocator = std::make_unique<TD3D12MultiBuddyAllocator>(InDevice, InitData);

	D3DDevice = InDevice;
}

void* TD3D12UploadBufferAllocator::AllocUploadResource(uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
	Allocator->AllocResource(Size, Alignment, ResourceLocation);

	return ResourceLocation.MappedAddress;
}

void TD3D12UploadBufferAllocator::CleanUpAllocations()
{
	Allocator->CleanUpAllocations();
}

TD3D12DefaultBufferAllocator::TD3D12DefaultBufferAllocator(ID3D12Device* InDevice)
{
	{
		TD3D12BuddyAllocator::TAllocatorInitData InitData;
		InitData.AllocatioStrategy = TD3D12BuddyAllocator::EAllocationStrategy::ManualSubAllocation;
		InitData.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		InitData.ResourceFlags = D3D12_RESOURCE_FLAG_NONE;

		Allocator = std::make_unique<TD3D12MultiBuddyAllocator>(InDevice, InitData);
	}

	{
		TD3D12BuddyAllocator::TAllocatorInitData InitData;
		InitData.AllocatioStrategy = TD3D12BuddyAllocator::EAllocationStrategy::ManualSubAllocation;
		InitData.HeapType = D3D12_HEAP_TYPE_DEFAULT;
		InitData.ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // for UAV

		UavAllocator = std::make_unique<TD3D12MultiBuddyAllocator>(InDevice, InitData);
	}

	D3DDevice = InDevice;
}

void TD3D12DefaultBufferAllocator::AllocDefaultResource(const D3D12_RESOURCE_DESC& REsourceDesc, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
	if (REsourceDesc.Flags == D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS)
	{
		UavAllocator->AllocResource((uint32_t)REsourceDesc.Width, Alignment, ResourceLocation);
	}
	else
	{
		Allocator->AllocResource((uint32_t)REsourceDesc.Width, Alignment, ResourceLocation);
	}
}

void TD3D12DefaultBufferAllocator::CleanUpAllocations()
{
	Allocator->CleanUpAllocations();
}

TD3D12TextureResourceAllocator::TD3D12TextureResourceAllocator(ID3D12Device* InDevice)
{
	TD3D12BuddyAllocator::TAllocatorInitData InitData;
	
	// placed resource : differents states for different resources
	InitData.AllocatioStrategy = TD3D12BuddyAllocator::EAllocationStrategy::PlacedResource;
	InitData.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	InitData.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
	
	Allocator = std::make_unique<TD3D12MultiBuddyAllocator>(InDevice, InitData);

	D3DDevice = InDevice;
}

void TD3D12TextureResourceAllocator::AllocTextureResource(const D3D12_RESOURCE_STATES& ResourceState, const D3D12_RESOURCE_DESC& ResourceDesc, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
	const D3D12_RESOURCE_ALLOCATION_INFO Info = D3DDevice->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	Allocator->AllocResource((uint32_t)Info.SizeInBytes, Alignment, ResourceLocation);

	// create placed resource
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;

		ID3D12Heap* BackingHeap = ResourceLocation.Allocator->GetBackingHeap();
		uint64_t HeapOffset = ResourceLocation.OffsetFromBaseOfHeap;
		D3DDevice->CreatePlacedResource(BackingHeap, HeapOffset, &ResourceDesc, ResourceState, nullptr, IID_PPV_ARGS(&Resource));

		TD3D12Resource* NewResource = new TD3D12Resource(Resource);
		ResourceLocation.UnderlyingResource = NewResource;
		ResourceLocation.BlockData.PlacedResource = NewResource; // will delete Resource when ResourceLocation was destoryed
	}
}

void TD3D12TextureResourceAllocator::CleanUpAllocations()
{
	Allocator->CleanUpAllocations();
}

TD3D12PixelResourceAllocator::TD3D12PixelResourceAllocator(ID3D12Device* InDevice)
{
	TD3D12BuddyAllocator::TAllocatorInitData InitData;

	// placed resource : differents states for different resources
	InitData.AllocatioStrategy = TD3D12BuddyAllocator::EAllocationStrategy::PlacedResource;
	InitData.HeapType = D3D12_HEAP_TYPE_DEFAULT;
	InitData.HeapFlags = D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES;

	Allocator = std::make_unique<TD3D12MultiBuddyAllocator>(InDevice, InitData);

	D3DDevice = InDevice;
}

void TD3D12PixelResourceAllocator::AllocTextureResource(const D3D12_RESOURCE_STATES& ResourceState, const D3D12_RESOURCE_DESC& ResourceDesc, uint32_t Alignment, D3D12_CLEAR_VALUE ClearValue, TD3D12ResourceLocation& ResourceLocation)
{
	const D3D12_RESOURCE_ALLOCATION_INFO Info = D3DDevice->GetResourceAllocationInfo(0, 1, &ResourceDesc);

	Allocator->AllocResource((uint32_t)Info.SizeInBytes, Alignment, ResourceLocation);

	// create placed resource
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;

		ID3D12Heap* BackingHeap = ResourceLocation.Allocator->GetBackingHeap();
		uint64_t HeapOffset = ResourceLocation.OffsetFromBaseOfHeap;
		D3DDevice->CreatePlacedResource(BackingHeap, HeapOffset, &ResourceDesc, ResourceState, &ClearValue, IID_PPV_ARGS(&Resource));

		TD3D12Resource* NewResource = new TD3D12Resource(Resource);
		ResourceLocation.UnderlyingResource = NewResource;
		ResourceLocation.BlockData.PlacedResource = NewResource; // will delete Resource when ResourceLocation was destoryed
	}
}

void TD3D12PixelResourceAllocator::CleanUpAllocations()
{
	Allocator->CleanUpAllocations();
}
