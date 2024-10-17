#pragma once

#include "D3D12Resource.h"
#include "D3D12MemoryAllocator.h"
#include <memory>

class TD3D12Buffer
{
public:
	TD3D12Buffer() {}
	virtual ~TD3D12Buffer() {}

	TD3D12Resource* GetResource() { return ResourceLocation.UnderlyingResource; }

public:
	TD3D12ResourceLocation ResourceLocation;
};

class TD3D12VertexBuffer : public TD3D12Buffer
{

};
typedef std::shared_ptr<TD3D12VertexBuffer> TD3D12VertexBufferRef;

class TD3D12IndexBuffer : public TD3D12Buffer
{

};
typedef std::shared_ptr<TD3D12IndexBuffer> TD3D12IndexBufferRef;

namespace TD3D12RHI
{
	extern std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator;

	extern std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator;

	void InitialzeAllocator(ID3D12Device* Device);

	TD3D12VertexBufferRef CreateVertexBuffer(const void* Contents, uint32_t Size, ID3D12GraphicsCommandList* cmdlist);

	TD3D12IndexBufferRef CreateIndexBuffer(const void* Contents, uint32_t Size);

	void CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, TD3D12ResourceLocation& ResourceLocation);
	
	void CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation, ID3D12GraphicsCommandList* cmdlist);
}