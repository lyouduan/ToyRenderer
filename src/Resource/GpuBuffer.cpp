#include "GpuBuffer.h"
#include "DXSamplerHelper.h"
#include "D3D12RHI.h"

D3D12_RESOURCE_DESC GpuBuffer::DescribeBuffer()
{
    assert(m_BufferSize != 0);

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Alignment = 0;
    Desc.DepthOrArraySize = 1;
    Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    Desc.Flags = m_ResourceFlags;
    Desc.Format = DXGI_FORMAT_UNKNOWN;
    Desc.Height = 1;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Desc.MipLevels = 1;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Width = (UINT64)m_BufferSize;

    return Desc;
}

void GpuBuffer::Create(const std::wstring& name, uint32_t NumElements, uint32_t ElementSize, const void* InitialData)
{
    Destroy();

    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    // D3D12_RESOURCE_DESC::Buffer(Size)
    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

    m_UsageState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_HEAP_PROPERTIES HeapProps;
    HeapProps.Type = D3D12_HEAP_TYPE_DEFAULT; // only GPU read and write
    HeapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProps.CreationNodeMask = 1;
    HeapProps.VisibleNodeMask = 1;

    // create resource  
    ThrowIfFailed(m_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

    //----------------------------
    //if (initialData)

    // if having initialData, using uploadBuffer transfer data to defaultBuffer
    // 1st. mapping data to uploadBuffer
    // 2nd uploadbuffer to defaultbuffer


    // create views for binding pipelines
    CreateDerivedViews();
}

void GpuBuffer::CreatePlaced(const std::wstring& name, ID3D12Heap* pBackingHeap, uint32_t HeapOffset, uint32_t NumElements, uint32_t ElementSize, const void* initialData)
{
    m_ElementCount = NumElements;
    m_ElementSize = ElementSize;
    m_BufferSize = NumElements * ElementSize;

    D3D12_RESOURCE_DESC ResourceDesc = DescribeBuffer();

    m_UsageState = D3D12_RESOURCE_STATE_COMMON;

    // create resource using explicit heap
    ThrowIfFailed(m_Device->CreatePlacedResource(pBackingHeap, HeapOffset, &ResourceDesc, m_UsageState, nullptr, IID_PPV_ARGS(&m_pResource)));

    m_GpuVirtualAddress = m_pResource->GetGPUVirtualAddress();

    //----------------------------
    //if (initialData)

    // if having initialData, using uploadBuffer transfer data to defaultBuffer
    // 1st. mapping data to uploadBuffer
    // 2nd uploadbuffer to defaultbuffer

    CreateDerivedViews();
}

D3D12_CPU_DESCRIPTOR_HANDLE GpuBuffer::CreateConstantBufferView(uint32_t Offset, uint32_t Size) const
{
    //assert(Offset + Size <= m_BufferSize);
    //
    //size_t alignment = 16;
    //Size = (Size + alignment - 1) & ~(alignment - 1);
    //
    //D3D12_CONSTANT_BUFFER_VIEW_DESC CBVDesc;
    //CBVDesc.BufferLocation = m_GpuVirtualAddress + (size_t)Offset;
    //CBVDesc.SizeInBytes = Size;

    //D3D12_CPU_DESCRIPTOR_HANDLE hCBV = AllocateDescriptor(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    //m_Device->CreateConstantBufferView(&CBVDesc, hCBV);
    //return hCBV;
}

D3D12_VERTEX_BUFFER_VIEW GpuBuffer::VertexBufferView(size_t Offset, uint32_t Size, uint32_t Stride) const
{
    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = m_GpuVirtualAddress + Offset;
    VBView.SizeInBytes = Size;
    VBView.StrideInBytes = Stride;
    return VBView;
}

D3D12_INDEX_BUFFER_VIEW GpuBuffer::IndexBufferView(size_t Offset, uint32_t Size, bool b32Bit) const
{
    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = m_GpuVirtualAddress + Offset;
    IBView.Format = b32Bit ? DXGI_FORMAT_R32_UINT : DXGI_FORMAT_R16_UINT;
    IBView.SizeInBytes = Size;
    return IBView;
}

void ByteAddressBuffer::CreateDerivedViews()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;

    if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_SRV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    m_Device->CreateShaderResourceView(m_pResource.Get(), &SRVDesc, m_SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
    UAVDesc.Buffer.NumElements = (UINT)m_BufferSize / 4;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

    if(m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_UAV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    m_Device->CreateUnorderedAccessView(m_pResource.Get(), nullptr, &UAVDesc, m_UAV);
}

void StructuredBuffer::CreateDerivedViews()
{
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.Buffer.NumElements = m_ElementCount;
    SRVDesc.Buffer.StructureByteStride = m_ElementSize;
    SRVDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

    if (m_SRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_SRV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    UAVDesc.Buffer.CounterOffsetInBytes = 0;
    UAVDesc.Buffer.NumElements = m_ElementCount;
    UAVDesc.Buffer.StructureByteStride = m_ElementSize;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    m_CounterBuffer.Create(L"StructuredBuffer::Counter", 1, 4);

    if (m_UAV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_UAV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    m_Device->CreateUnorderedAccessView(m_pResource.Get(), m_CounterBuffer.GetResource(), &UAVDesc, m_UAV);
}
