#pragma once

#include "D3D12Resource.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12HeapSlotAllocator.h"
#include "D3D12DescriptorCache.h"
#include "D3D12CommandContext.h"

#include <memory>

namespace TD3D12RHI
{
	extern ID3D12Device* g_Device;
	extern TD3D12CommandContext g_CommandContext;
	extern Microsoft::WRL::ComPtr<IDXGISwapChain1> g_SwapCHain;

	// memory allocator
	extern std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator;
	extern std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator;
	extern std::unique_ptr<TD3D12TextureResourceAllocator> TextureResourceAllocator;

	// heapSlot allocator
	extern std::unique_ptr<TD3D12HeapSlotAllocator> RTVHeapSlotAllocator;

	extern std::unique_ptr<TD3D12HeapSlotAllocator> DSVHeapSlotAllocator;

	extern std::unique_ptr<TD3D12HeapSlotAllocator> SRVHeapSlotAllocator;

	// cache descriptor for GPU
	extern std::unique_ptr<TD3D12DescriptorCache> DescriptorCache;

	void Initialze();

	void InitialzeAllocator();

	TD3D12VertexBufferRef CreateVertexBuffer(const void* Contents, uint32_t Size);

	TD3D12IndexBufferRef CreateIndexBuffer(const void* Contents, uint32_t Size);

	void CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, TD3D12ResourceLocation& ResourceLocation);

	void CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation);

	void InitializeTexture(TD3D12Resource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);

	TD3D12HeapSlotAllocator* GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);
}