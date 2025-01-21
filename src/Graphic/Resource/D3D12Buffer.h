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

class TD3D12ConstantBuffer : public TD3D12Buffer
{
public:
	void CreateDerivedViews(uint32_t Size);

private:
	D3D12_CONSTANT_BUFFER_VIEW_DESC CBV_Desc;
};

typedef std::shared_ptr<TD3D12ConstantBuffer> TD3D12ConstantBufferRef;

class TD3D12StructuredBuffer : public TD3D12Buffer
{
public:
	D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV()
	{
		return SRV;
	}

	void CreateDerivedView(uint32_t Elementsize, uint32_t ElementCount);

private:

	D3D12_CPU_DESCRIPTOR_HANDLE SRV;
};

typedef std::shared_ptr<TD3D12StructuredBuffer> TD3D12StructuredBufferRef;


class TD3D12RWStructuredBuffer : public TD3D12Buffer
{
public:
	
	D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV()
	{
		return SRV;
	}

	D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV()
	{
		return UAV;
	}

	void CreateDerivedViews(uint32_t Elementsize, uint32_t ElementCount);

private:
	D3D12_CPU_DESCRIPTOR_HANDLE SRV;
	D3D12_CPU_DESCRIPTOR_HANDLE UAV;
};
typedef std::shared_ptr<TD3D12RWStructuredBuffer> TD3D12RWStructuredBufferRef;


class TD3D12ReadBackBuffer : public TD3D12Buffer
{

};
typedef std::shared_ptr<TD3D12ReadBackBuffer> TD3D12ReadBackBufferRef;