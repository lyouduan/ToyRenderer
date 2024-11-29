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
	// 显存分配Allocation
	TD3D12ResourceLocation ResourceLocation;
};

class TD3D12VertexBuffer : public TD3D12Buffer
{
public:
	const D3D12_VERTEX_BUFFER_VIEW& GetVBV() const { return m_VBV; }
	void CreateDerivedViews(uint32_t Size, UINT Stride);

private:

	D3D12_VERTEX_BUFFER_VIEW m_VBV;
};
typedef std::shared_ptr<TD3D12VertexBuffer> TD3D12VertexBufferRef;

class TD3D12IndexBuffer : public TD3D12Buffer
{
public:

	const D3D12_INDEX_BUFFER_VIEW& GetIBV() const { return m_IBV; }
	void CreateDerivedViews(uint32_t Size, DXGI_FORMAT Format);

private:
	D3D12_INDEX_BUFFER_VIEW m_IBV;
};
typedef std::shared_ptr<TD3D12IndexBuffer> TD3D12IndexBufferRef;

