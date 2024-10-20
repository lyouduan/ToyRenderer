#include "UploadBuffer.h"
#include "DXSamplerHelper.h"

#ifndef minX
#define minX(a,b)            (((a) < (b)) ? (a) : (b))
#endif

void UploadBuffer::Create(ID3D12Device* device, const std::wstring& name, size_t BufferSize)
{
	Destroy();

	m_BufferSize = BufferSize;

	// create an upload buffer
	D3D12_HEAP_PROPERTIES HeapProps;
	HeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
	HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	HeapProps.CreationNodeMask = 1;
	HeapProps.VisibleNodeMask = 1;

	// upload buffers must bve 1-dimensional
	D3D12_RESOURCE_DESC ResourceDesc = {};
	ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	ResourceDesc.Width = m_BufferSize;
	ResourceDesc.Height = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.DepthOrArraySize = 1;
	ResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	ResourceDesc.SampleDesc.Count = 1;
	ResourceDesc.SampleDesc.Quality = 0;
	ResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	ThrowIfFailed(device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_pResource)));

	m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();
}

void* UploadBuffer::Map()
{
	void* MappedAddress;
	auto range = CD3DX12_RANGE(0, m_BufferSize);
	m_pResource->Map(0, &range, &MappedAddress);

	return MappedAddress;
}

void UploadBuffer::Unmap(size_t begin, size_t end)
{
	auto range = CD3DX12_RANGE(begin, minX(end, m_BufferSize));
	m_pResource->Unmap(0, &range);
}
