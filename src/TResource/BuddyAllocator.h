#pragma once
#include "GpuBuffer.h"
#include <queue>
#include <set>
#define MIN_PLACED_BUFFER_SIZE (64 * 1024)

enum kBuddyAllocationStrategy
{
	// This strategy uses Placed Resources to sub-allocate a buffer out of an underlying ID3D12Heap
	// The benefit of this is that each buffer can have it's own resource state 
	// and can be treated as any other buffer. The downside of this strategy is the API limitation which enforecs
	// the minium buffer size to 64k leading to large internal fragmentation in the allocator
	kPlacedResourceStrategy,

	// The alternative is to manualy sub-allocate out of a single large buffer which allows block
	// allocation granularity down to 1 byte.
	// However, this strategy is only really valid for buffers which will be treated as read-only after their creation
	// (i.e. most Index and Vertex buffers).
	// This is because the underlying resource can only have one state at a time
	kManualSubAllocationStrategy
};


struct BuddyBlock
{
	// cite for allocated-obj
	ByteAddressBuffer* m_pBuffer; // Resource
	ID3D12Heap* m_pBackingHeap; // Heap

	size_t m_offset;
	size_t m_size;
	size_t m_unpaddedSize;
	uint64_t m_fenceValue;

	inline size_t GetOffset() const { return m_offset; }
	inline size_t GetSize() const { return m_size; }

	BuddyBlock() : m_pBuffer(nullptr), m_pBackingHeap(nullptr), m_offset(0), m_size(0), m_unpaddedSize(0), m_fenceValue(0)
	{
	}

	BuddyBlock(uint32_t heapOffset, uint32_t totalSize, uint32_t unpaddedSize);

	void InitPlaced(ID3D12Heap* pBackingHeap, uint32_t numElments, uint32_t elementSize, const void* initialData = nullptr);

	void InitFromResource(ByteAddressBuffer* pBuffer, uint32_t numElements, uint32_t elementSize, const void* initialData = nullptr);

	void Destroy();
};

class BuddyAllocator
{
public:
	BuddyAllocator(kBuddyAllocationStrategy allocationStrategy, D3D12_HEAP_TYPE heapType, size_t maxBlockSize, size_t minBlockSize = MIN_PLACED_BUFFER_SIZE, size_t baseOffset = 0);

	void Initialize();

	void Destroy();

	BuddyBlock* Allocate(uint32_t numElments, uint32_t elementSize, const void* initialData = nullptr);

	void Deallocate(BuddyBlock* pBlock);

	inline bool IsOwner(const BuddyBlock& block)
	{
		return block.GetOffset() >= m_baseOffset && block.GetSize() <= m_maxBlockSize;
	}

	inline void Reset()
	{
		// clear the free blocks collection
		m_freeBlocks.clear();
		m_freeBlocks.resize(m_maxOrder + 1);
		m_freeBlocks[m_maxOrder].insert((size_t)0);
	}

	void CleanUpAllocations();

private:
	ID3D12Heap* m_pBackingHeap;
	ByteAddressBuffer m_BackingResource;

	const D3D12_HEAP_TYPE m_heapType;

	std::queue<BuddyBlock*> m_deferredDeletionQueue;
	std::vector<std::set<size_t>> m_freeBlocks;

	UINT m_maxOrder;

	const size_t m_baseOffset;
	const size_t m_maxBlockSize;
	const size_t m_minBlockSize;

	const kBuddyAllocationStrategy m_allocationStrategy;

	inline size_t SzieToUnitSize(size_t size) const
	{
		return (size + (m_minBlockSize - 1)) / m_minBlockSize;
	}

	inline UINT UnitSizeToOrder(size_t size)  const
	{
		unsigned long Result;
		_BitScanReverse(&Result, size + size - 1);// ceil(log2(Size))
		return Result;
	}

	inline size_t GetBuddyOffset(const size_t& offset, const size_t& size)
	{
		return offset ^ size;
	}

	void DeallocateInternal(BuddyBlock* pBlock);

	size_t OrderToUnitSize(UINT order) const { return ((size_t)1) << order; }

	size_t AllocateBlock(UINT order);
	void DeallcateBlock(size_t offset, UINT order);

	size_t m_SpaceUsed;
	size_t m_InternalFragmentation;
};
