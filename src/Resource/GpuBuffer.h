#pragma once
#include "GpuResource.h"


// default buffer
//
class GpuBuffer : public GpuResource
{
public:
	virtual ~GpuBuffer() { Destroy(); }

	// create a buffer
	void Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const void* InitialData = nullptr);

	//void create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const )

	// Sub-Allocate a buffer out of a pre-allocated heap.
	void CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize,
		const void* initialData = nullptr);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_UAV; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRV; }

	D3D12_GPU_VIRTUAL_ADDRESS RootConstantBufferView() const { return m_GpuVirtualAddress; }

	D3D12_CPU_DESCRIPTOR_HANDLE CreateConstantBufferView(uint32_t Offset, uint32_t Size) const;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const;

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView(size_t BaseVertexIndex = 0) const
	{
		size_t Offset = BaseVertexIndex * m_ElementSize;

		return VertexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementCount);
	}

	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit = false) const;

	D3D12_INDEX_BUFFER_VIEW IndexBufferView(size_t StartIndex = 0) const
	{
		size_t Offset = StartIndex * m_ElementSize;

		return IndexBufferView(Offset, (uint32_t)(m_BufferSize - Offset), m_ElementSize == 4);
	}

	size_t GetBufferSize() const { return m_BufferSize; }
	uint32_t GetElementCount() const { return m_ElementCount; }
	uint32_t GetElmentSize() const { return m_ElementSize; }

protected:

	GpuBuffer(ID3D12Device* InDevice) : m_Device(InDevice), m_BufferSize(0), m_ElementCount(0), m_ElementSize(0)
	{
		m_ResourceFlags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		m_UAV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_SRV.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	ID3D12Device* m_Device;

	D3D12_RESOURCE_DESC DescribeBuffer();

	virtual void CreateDerivedViews() = 0; // pure virtual function

	// handle for binding Root Signature
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAV;
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRV;

	size_t m_BufferSize;
	uint32_t m_ElementCount;
	uint32_t m_ElementSize;
	D3D12_RESOURCE_FLAGS m_ResourceFlags;
};

// 字节地址缓冲区
class ByteAddressBuffer : public GpuBuffer
{
public:
	virtual void CreateDerivedViews() override;
};

class StructuredBuffer : public GpuBuffer
{
public:
	virtual void Destroy() override
	{
		m_CounterBuffer.Destroy();
		GpuBuffer::Destroy();
	}

	virtual void CreateDerivedViews() override;

	ByteAddressBuffer& GetCounterBuffer() { return m_CounterBuffer; }

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterSRV() { return m_CounterBuffer.GetSRV(); };
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetCounterUAV() { return m_CounterBuffer.GetUAV(); };

private:
	ByteAddressBuffer m_CounterBuffer;
};

