#pragma once

#include "D3D12Resource.h"
#include "D3D12MemoryAllocator.h"
#include "D3D12Buffer.h"
#include "D3D12HeapSlotAllocator.h"
#include "D3D12DescriptorCache.h"
#include "D3D12CommandContext.h"
#include "D3D12PixelBuffer.h"

#include <memory>
#define FrameCount 2

namespace TD3D12RHI
{
	extern ID3D12Device* g_Device;
	extern TD3D12CommandContext g_CommandContext;
	extern Microsoft::WRL::ComPtr<IDXGISwapChain1> g_SwapCHain;
	extern Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_ImGuiSrvHeap;

	extern uint32_t g_DisplayWidth;
	extern uint32_t g_DisplayHeight;

	// Buffer
	extern D3D12DepthBuffer g_DepthBuffer;

	// memory allocator
	extern std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator;
	extern std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator;
	extern std::unique_ptr<TD3D12TextureResourceAllocator> TextureResourceAllocator;

	// heapSlot allocator
	extern std::unique_ptr<TD3D12HeapSlotAllocator> RTVHeapSlotAllocator;
	extern std::unique_ptr<TD3D12HeapSlotAllocator> DSVHeapSlotAllocator;
	extern std::unique_ptr<TD3D12HeapSlotAllocator> SRVHeapSlotAllocator;
	extern std::unique_ptr<TD3D12HeapSlotAllocator> ImGuiSRVHeapAllocator;

	// cache descriptor for GPU
	extern std::unique_ptr<TD3D12DescriptorCache> DescriptorCache;

	void InitialzeAllocator();
	void InitialzeBuffer();
	void Initialze();

	TD3D12VertexBufferRef CreateVertexBuffer(const void* Contents, uint32_t Size, uint32_t Stride);

	TD3D12IndexBufferRef CreateIndexBuffer(const void* Contents, uint32_t Size, DXGI_FORMAT Format);

	TD3D12ConstantBufferRef CreateConstantBuffer(const void* Contents, uint32_t Size);

	void CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, TD3D12ResourceLocation& ResourceLocation);

	void CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation);

	void InitializeTexture(TD3D12Resource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[]);
	void UploadTextureData(TD3D12Resource TextureResource, const std::vector<D3D12_SUBRESOURCE_DATA>& InitData);

	TD3D12HeapSlotAllocator* GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE HeapType);

	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetImGuiSRVHeapAllocator();
}