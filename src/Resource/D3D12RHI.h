#pragma once

#include "D3D12Resource.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12HeapSlotAllocator.h"
#include "D3D12DescriptorCache.h"

#include <memory>

namespace TD3D12RHI
{
	// memory allocator
	extern std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator;
	extern std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator;

	// heapSlot allocator
	extern std::unique_ptr<TD3D12HeapSlotAllocator> RTVHeapSlotAllocator;

	extern std::unique_ptr<TD3D12HeapSlotAllocator> DSVHeapSlotAllocator;

	extern std::unique_ptr<TD3D12HeapSlotAllocator> SRVHeapSlotAllocator;

	// cache descriptor for GPU
	extern std::unique_ptr<TD3D12DescriptorCache> DescriptorCache;

	void InitialzeAllocator(ID3D12Device* Device);

	TD3D12VertexBufferRef CreateVertexBuffer(const void* Contents, uint32_t Size, ID3D12GraphicsCommandList* cmdlist);

	TD3D12IndexBufferRef CreateIndexBuffer(const void* Contents, uint32_t Size, ID3D12GraphicsCommandList* cmdlist);

	void CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, TD3D12ResourceLocation& ResourceLocation);

	void CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation, ID3D12GraphicsCommandList* cmdlist);

	TD3D12HeapSlotAllocator* GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
}