#include "D3D12MemoryAllocator.h"
#include "stdafx.h"

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
		// Allocate block
		const uint32_t UnitSize = SizeToUnitSize(SizeToAllocate);
		const uint32_t Order = UnitSizeToOrder(UnitSize);
		const uint32_t Offset = AllocateBlock(Order); // this is the offset in MinBlockSize units

		// 记录对齐后分配的容量大小
		const uint32_t AllocSize = UnitSize * MinBlockSize;
		TotalAllocSize += AllocSize;

		// Calculate AlignedOffsetFromResourceBase
		const uint32_t OffsetFromBaseOfResource = GetAllocOffsetInBytes(Offset);
		uint32_t AlignedOffsetFromResourceBase = OffsetFromBaseOfResource;
		if (Alignment != 0 && OffsetFromBaseOfResource % Alignment != 0)
		{
			// 向上对齐
			AlignedOffsetFromResourceBase = AlignArbitrary(OffsetFromBaseOfResource, Alignment);

			// 计算对齐所需的补充大小
			uint32_t Padding = AlignedOffsetFromResourceBase - OffsetFromBaseOfResource;
			assert((Padding + Size) <= AllocSize);
		}
		assert((AlignedOffsetFromResourceBase % Alignment) == 0);

		// save allocation info to ResourceLocation
		ResourceLocation.SetType(TD3D12ResourceLocation::EResourceLocationType::SubAlloction);
		ResourceLocation.BlockData.Order = Order;
		ResourceLocation.BlockData.Offset = Offset;
		ResourceLocation.BlockData.ActualUsedSize = Size;
		ResourceLocation.Allocator = this;

		if (InitData.AllocationStrategy == EAllocationStrategy::ManualSubAlloction)
		{
			ResourceLocation.UnderlyingResource = BackingResource;
			ResourceLocation.OffsetFromBaseOfResource = AlignedOffsetFromResourceBase;
			ResourceLocation.GPUVirtualAddress = BackingResource->GPUVirtualAddress + AlignedOffsetFromResourceBase;

			if (InitData.HeapType == D3D12_HEAP_TYPE_UPLOAD)
			{
				ResourceLocation.MappedAddress = ((uint8_t*)BackingResource->MappedBaseAddress + AlignedOffsetFromResourceBase);
			}
		}
		else// EAllocationStrategy::PlacedResource
		{
			ResourceLocation.OffsetFromBaseOfHeap = AlignedOffsetFromResourceBase;

			// Place resource are initialized by caller
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
	// 延迟回收（一帧结束时）
	DeferredDeletionQueue.push_back(ResourceLocation.BlockData);
}

void TD3D12BuddyAllocator::CleanUpAllocation()
{
	for (int32_t i = 0; i < DeferredDeletionQueue.size(); i++)
	{
		const TD3D12BuddyBlockData& Block = DeferredDeletionQueue[i];

		DeallocateInternal(Block);
	}

	// clear out all of the released blocks, don't allow the array to shrink
	DeferredDeletionQueue.clear();
}

void TD3D12BuddyAllocator::Initialize()
{
	// create BackingHeap or BackingResource
	if (InitData.AllocationStrategy == EAllocationStrategy::PlacedResource)
	{
		CD3DX12_HEAP_PROPERTIES HeapProperties(InitData.HeapType);

		D3D12_HEAP_DESC Desc = {};
		Desc.SizeInBytes = DEFAULT_POOL_SIZE;
		Desc.Properties = HeapProperties;
		Desc.Alignment = 0;
		Desc.Flags = InitData.HeapFlags;

		// create backingHeap, we will create place resource on it
		ID3D12Heap* Heap = nullptr;

		ThrowIfFailed(D3DDevice->CreateHeap(&Desc, IID_PPV_ARGS(&Heap)));
		Heap->SetName(L"TD3D12BuddyAllocator BackingHeap");

		// 指向该资源堆
		BackingHeap = Heap;
	}
	else // ManualSubAllocation
	{
		CD3DX12_HEAP_PROPERTIES HeapProperties(InitData.HeapType);
		D3D12_RESOURCE_STATES HeapResourceStates;
		if (InitData.HeapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			HeapResourceStates = D3D12_RESOURCE_STATE_GENERIC_READ;
		}
		else// Default heap
		{
			HeapResourceStates = D3D12_RESOURCE_STATE_COMMON;
		}

		CD3DX12_RESOURCE_DESC BufferDesc = CD3DX12_RESOURCE_DESC::Buffer(DEFAULT_POOL_SIZE, InitData.ResourceFlags);

		// create committed resource, we will allocate sub-regions on it
		Microsoft::WRL::ComPtr<ID3D12Resource> Resource;
		ThrowIfFailed(D3DDevice->CreateCommittedResource(// 创建显存资源，隐式堆
			&HeapProperties,
			D3D12_HEAP_FLAG_NONE,
			&BufferDesc,
			HeapResourceStates,
			nullptr,
			IID_PPV_ARGS(&Resource)));

		Resource->SetName(L"TD3D12BuddyAllocator BackingResource");

		BackingResource = new TD3D12Resource(Resource);

		// Mapping for upload buffer
		if (InitData.HeapType == D3D12_HEAP_TYPE_UPLOAD)
		{
			BackingResource->Map();
		}
	}

	// Initialize free blocks, add the free block for MaxOrder
	MaxOrder = UnitSizeToOrder(SizeToUnitSize(DEFAULT_POOL_SIZE));

	// 为各阶创建空闲块
	for (uint32_t i = 0; i <= MaxOrder; ++i)
	{
		FreeBlocks.emplace_back(std::set<uint32_t>());
	}

	// 最高阶的空闲块，其起始地址设为0
	FreeBlocks[MaxOrder].insert((uint32_t)0);
}

uint32_t TD3D12BuddyAllocator::AllocateBlock(uint32_t Order)
{
	uint32_t Offset;

	// 递归退出条件，防止数组越界，超过最大阶数
	if (Order > MaxOrder)
	{
		assert(false);
	}

	if (FreeBlocks[Order].size() == 0) // 该阶数无空闲块， 往上一阶找
	{
		// No free nodes in the requested pool. Try to find a higher-order block and split it
		// 递归，得到当前阶数的偏移 
		uint32_t Left = AllocateBlock(Order + 1);

		uint32_t UnitSize = OrderToUnitSize(Order); // 当前阶数转换成单位作

		uint32_t Right = Left + UnitSize; // 即偏移一半

		FreeBlocks[Order].insert(Right); // add the right block to the free pool

		Offset = Left; // return left block
	}
	else // 有空闲块
	{
		auto It = FreeBlocks[Order].cbegin(); // 获取第一个元素，即最小偏移量的空闲块
		Offset = *It;

		// Remove the block from the free list
		FreeBlocks[Order].erase(*It);
	}

	return Offset;
}

uint32_t TD3D12BuddyAllocator::GetSizeToAllocate(uint32_t Size, uint32_t Alignment)
{
	uint32_t SizeToAllocate = Size;

	// If the alignment doesn't match the block size
	if (Alignment != 0 && MinBlockSize % Alignment != 0)
	{
		SizeToAllocate = Size + Alignment;
	}

	return SizeToAllocate;
}

bool TD3D12BuddyAllocator::CanAllocate(uint32_t SizeToAllocate)
{
	// 已分配总容量 == 默认容量
	if (TotalAllocSize == DEFAULT_POOL_SIZE)
	{
		return false;
	}

	// 从最大容量空闲块开始遍历
	uint32_t BlockSize = DEFAULT_POOL_SIZE;
	for (int i = (int)FreeBlocks.size() - 1; i >= 0; i--)
	{
		if (FreeBlocks[i].size() && BlockSize >= SizeToAllocate)
		{
			return true;
		}

		// 往下一阶数找，容量减半
		// Halve the block size
		BlockSize = BlockSize >> 1;

		// 减半后不足分配容量
		if (BlockSize < SizeToAllocate) return false;
	}

	return false;
}

void TD3D12BuddyAllocator::DeallocateInternal(const TD3D12BuddyBlockData& Block)
{
	// 释放中间的块
	DeallocateBlock(Block.Offset, Block.Order);

	// 扣除已分配容量
	uint32_t Size = OrderToUnitSize(Block.Order) * MinBlockSize;
	TotalAllocSize -= Size;

	// 如果是place resource, 释放关联资源
	if (InitData.AllocationStrategy == EAllocationStrategy::PlacedResource)
	{
		// release place resource
		assert(Block.PlacedResource != nullptr);

		delete Block.PlacedResource;
	}
}

void TD3D12BuddyAllocator::DeallocateBlock(uint32_t Offset, uint32_t Order)
{
	// 判断该块伙伴节点是否空闲，归并成更大的块，然后继续向上归并

	// Get Buddy block
	uint32_t Size = OrderToUnitSize(Order);
	uint32_t Buddy = GetBuddyOffset(Offset, Size);

	auto It = FreeBlocks[Order].find(Buddy);
	if (It != FreeBlocks[Order].end()) // If buddy block is free ,merge it
	{
		// Deallocate merged blocks
		// 递归， 继续向上归并
		DeallocateBlock(min(Offset, Buddy), Order + 1);

		// remove the buddy from the free list
		FreeBlocks[Order].erase(*It);
	}
	else
	{
		// add the block to free list
		FreeBlocks[Order].insert(Offset);
	}
}
