#include "BuddyAllocator.h"
#include "stdafx.h"
#include "D3D12RHI.h"

BuddyBlock::BuddyBlock(uint32_t heapOffset, uint32_t totalSize, uint32_t unpaddedSize) :
	m_pBuffer(nullptr),
	m_pBackingHeap(nullptr),
	m_offset(heapOffset),
	m_size(totalSize),
	m_fenceValue(0),
	m_unpaddedSize(unpaddedSize)
{
}

void BuddyBlock::InitPlaced(ID3D12Heap* pBackingHeap, uint32_t numElments, uint32_t elementSize, const void* initialData)
{
	m_pBuffer = new ByteAddressBuffer();
	m_pBackingHeap = pBackingHeap;

	const std::wstring name(L"Buddy Bloack");
	m_pBuffer->CreatePlaced(name, m_pBackingHeap, uint32_t(m_offset), numElments, elementSize, initialData);
}

void BuddyBlock::InitFromResource(ByteAddressBuffer* pBuffer, uint32_t numElements, uint32_t elementSize, const void* initialData)
{
	m_pBuffer = pBuffer;

	if (initialData)
	{
		// initial Data
	}
}

void BuddyBlock::Destroy()
{
	m_pBuffer->Destroy();
	m_pBuffer = nullptr;
}

BuddyAllocator::BuddyAllocator(kBuddyAllocationStrategy allocationStrategy, D3D12_HEAP_TYPE heapType, size_t maxBlockSize, size_t minBlockSize, size_t baseOffset) :
	m_allocationStrategy(allocationStrategy),
	m_heapType(heapType),
	m_baseOffset(baseOffset),
	m_maxBlockSize(maxBlockSize),
	m_minBlockSize(minBlockSize),
	m_pBackingHeap(nullptr)
{
	m_maxOrder = UnitSizeToOrder(SzieToUnitSize(maxBlockSize));
	Reset();
}

void BuddyAllocator::Initialize()
{
	if (m_allocationStrategy == kBuddyAllocationStrategy::kPlacedResourceStrategy)
	{
		D3D12_HEAP_PROPERTIES heapProps = CD3DX12_HEAP_PROPERTIES(m_heapType);

		D3D12_HEAP_DESC desc = {};

		desc.SizeInBytes = m_maxBlockSize;
		desc.Properties = heapProps;
		desc.Alignment = MIN_PLACED_BUFFER_SIZE;
		desc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

		ThrowIfFailed(TD3D12RHI::g_Device->CreateHeap(&desc, IID_PPV_ARGS(&m_pBackingHeap)));
	}
	else
	{
		m_BackingResource.Create(L"Buddy Allocator Backing Resource", uint32_t(m_maxBlockSize), 1, nullptr);
	}
}

void BuddyAllocator::Destroy()
{
	if (m_allocationStrategy == kBuddyAllocationStrategy::kPlacedResourceStrategy)
	{
		m_pBackingHeap->Release();
	}
	else
	{
		m_BackingResource.Destroy();
	}
}

BuddyBlock* BuddyAllocator::Allocate(uint32_t numElements, uint32_t elementSize, const void* initialData)
{
	size_t size = numElements * elementSize;
	size_t unitSize = SzieToUnitSize(size);
	UINT order = UnitSizeToOrder(unitSize);

	try
	{
		size_t offset = AllocateBlock(order);

		uint32_t paddedSize = uint32_t(OrderToUnitSize(order) * m_minBlockSize);
		uint32_t blockOffset = uint32_t(m_baseOffset + (offset * m_minBlockSize));

		m_SpaceUsed += paddedSize;
		m_InternalFragmentation -= (paddedSize - size);

		BuddyBlock* pBlock = new BuddyBlock(blockOffset, paddedSize, numElements * elementSize);

		if (m_allocationStrategy == kBuddyAllocationStrategy::kPlacedResourceStrategy)
		{
			pBlock->InitPlaced(m_pBackingHeap, numElements, elementSize, initialData);
		}
		else

		{
			pBlock->InitFromResource(&m_BackingResource, numElements, elementSize, initialData);
		}
		return pBlock;
	}
	catch (std::bad_alloc&)
	{
		// There are no blocks available for the requested size so  
		// return the NULL block type  
		return new BuddyBlock();
	}
}

void BuddyAllocator::Deallocate(BuddyBlock* pBlock)
{

}

void BuddyAllocator::CleanUpAllocations()
{
}

void BuddyAllocator::DeallocateInternal(BuddyBlock* pBlock)
{
}

size_t BuddyAllocator::AllocateBlock(UINT order)
{
	size_t offset;

	if (order > m_maxOrder)
	{
		throw(std::bad_alloc()); // can't allocate a block that large
	}

	auto it = m_freeBlocks[order].begin();

	if (it == m_freeBlocks[order].end())
	{
		// no free nodes in the requested pool. try to find a higher-order block and split it.
		size_t left = AllocateBlock(order + 1);
		size_t size = OrderToUnitSize(order);

		size_t right = left + size;

		m_freeBlocks[order].insert(right);//add the right block to the free pool

		offset = left; // return the left block
	}
	else
	{
		offset = *it;

		// remove the block from the free list
		m_freeBlocks[order].erase(it);
	}


	return offset;
}

void BuddyAllocator::DeallcateBlock(size_t offset, UINT order)
{
	//see if the buddy block is free
	size_t size = OrderToUnitSize(order);
	size_t buddy = GetBuddyOffset(offset, size);

	auto it = m_freeBlocks[order].find(buddy);

	if (it != m_freeBlocks[order].end())
	{
		// Deallocate merged blocks
		DeallcateBlock(min(offset, buddy), order + 1);
		m_freeBlocks[order].erase(it);
	}
	else
	{
		// add the block to the free list
		m_freeBlocks[order].insert(offset);
	}
}	




