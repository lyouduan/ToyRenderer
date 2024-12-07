#include "D3D12HeapSlotAllocator.h"
#include "DXSamplerHelper.h"

TD3D12HeapSlotAllocator::TD3D12HeapSlotAllocator(ID3D12Device* InDevice, D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptorPerHeap)
	: D3DDevice(InDevice), 
	HeapDesc(CreateHeapDesc(Type, NumDescriptorPerHeap)),
	DescriptorSize(D3DDevice->GetDescriptorHandleIncrementSize(HeapDesc.Type))
{
}

TD3D12HeapSlotAllocator::~TD3D12HeapSlotAllocator()
{
}

TD3D12HeapSlotAllocator::HeapSlot TD3D12HeapSlotAllocator::AllocateHeapSlot()
{
	// find the entry with free list
	int EntryIndex = -1;
	for (int i = 0; i < HeapMap.size(); ++i)
	{
		if (HeapMap[i].FreeList.size() > 0)
		{
			EntryIndex = i;
			break;
		}
	}

	// if all entry are full, create a new one
	if (EntryIndex == -1)
	{
		AllocateHeap();

		EntryIndex = int(HeapMap.size() - 1);
	}

	// allcate a slot based on EntryIndex
	HeapEntry& Entry = HeapMap[EntryIndex];
	assert(Entry.FreeList.size() > 0);

	FreeRange& Range = Entry.FreeList.front();
	HeapSlot Slot = { (uint32_t)EntryIndex, Range.Start };

	// Remove this range if all slot has been allocated
	Range.Start += DescriptorSize;
	if (Range.Start == Range.End)
	{
		Entry.FreeList.pop_front();
	}

	return Slot;
}

Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> TD3D12HeapSlotAllocator::AllocateHeapOnly()
{
	// create a new descriptorHeap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	D3D12_DESCRIPTOR_HEAP_DESC Desc = HeapDesc;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(&Heap)));
	SetDebugName(Heap.Get(), L"TD3D12Heap Descriptor Heap");

	// add the entry to heapMap
	return Heap;
}

D3D12_DESCRIPTOR_HEAP_DESC TD3D12HeapSlotAllocator::CreateHeapDesc(D3D12_DESCRIPTOR_HEAP_TYPE Type, uint32_t NumDescriptorPerHeap)
{
	D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
	Desc.Type = Type;
	Desc.NumDescriptors = NumDescriptorPerHeap;
	Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // This heap will not be bound to the shader
	Desc.NodeMask = 0;

	return Desc;
}

void TD3D12HeapSlotAllocator::AllocateHeap()
{
	// create a new descriptorHeap
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap;
	ThrowIfFailed(D3DDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(&Heap)));
	SetDebugName(Heap.Get(), L"TD3D12HeapSlotAllocator Descriptor Heap");

	// Add an entry covering the entire heap
	DescriptorHandle HeapBase = Heap->GetCPUDescriptorHandleForHeapStart();
	assert(HeapBase.ptr != 0);

	HeapEntry Entry;
	Entry.Heap = Heap;
	Entry.FreeList.push_back({ HeapBase.ptr, HeapBase.ptr + (SIZE_T)HeapDesc.NumDescriptors * DescriptorSize });

	// add the entry to heapMap
	HeapMap.push_back(Entry);
}

void TD3D12HeapSlotAllocator::FreeHeapSlot(const HeapSlot& Slot)
{
	assert(Slot.HeapIndex < HeapMap.size());

	HeapEntry& Entry = HeapMap[Slot.HeapIndex];

	FreeRange NewRange =
	{
		Slot.Handle.ptr,
		Slot.Handle.ptr + DescriptorSize
	};

	bool bFound = false;

	for (auto Node = Entry.FreeList.begin(); Node != Entry.FreeList.end() && !bFound; Node++)
	{
		FreeRange& Range = *Node;
		assert(Range.Start < Range.End);


		// ----------------------------------------
		//  NewRange.Start>>>>>>NewRange.End,Range.Start>>>>>>Range.End
		// ----------------------------------------
		if (Range.Start == NewRange.End) // Merge NewRange and Range
		{
			Range.Start = NewRange.End;
			bFound = true;
		}
		// ----------------------------------------
		//  Range.Start>>>>>>Range.End,NewRange.Start>>>>>>NewRange.End
		// ----------------------------------------
		else if (Range.End == NewRange.Start)
		{
			Range.End = NewRange.End;
			bFound = true;
		}
		// ----------------------------------------
		//  Range.Start>>>>>>Range.End >>>> NewRange.Start>>>>>>NewRange.End
		// ----------------------------------------
		//  Range.Start>>>>>>Range.End >>>> NewRange.Start>>>>>>NewRange.End
		// ----------------------------------------
		else
		{
			// Range在New前面
			assert(Range.End < NewRange.Start || Range.Start > NewRange.Start);

			if (Range.Start > NewRange.Start)// insert NewRange before Range
			{
				Entry.FreeList.insert(Node, NewRange);
				bFound = true;
			}
		}
	}

	if (!bFound)
	{
		// add NewRange to tail
		Entry.FreeList.push_back(NewRange);
	}
}
