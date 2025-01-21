#include "D3D12Buffer.h"
#include "stdafx.h"
#include "D3D12RHI.h"
#include "D3D12Resource.h"

TD3D12VertexBufferRef TD3D12RHI::CreateVertexBuffer(const void* Contents, uint32_t Size, uint32_t Stride)
{
    TD3D12VertexBufferRef VertexBufferRef = std::make_shared<TD3D12VertexBuffer>();

    CreateAndInitDefaultBuffer(Contents, Size, DEFAULT_RESOURCE_ALIGNMENT, VertexBufferRef->ResourceLocation);
    // create VBV
    VertexBufferRef->CreateDerivedViews(Size, Stride);
    return VertexBufferRef;
}

TD3D12IndexBufferRef TD3D12RHI::CreateIndexBuffer(const void* Contents, uint32_t Size, DXGI_FORMAT Format)
{
    TD3D12IndexBufferRef IndexBufferRef = std::make_shared<TD3D12IndexBuffer>();

    CreateAndInitDefaultBuffer(Contents, Size, DEFAULT_RESOURCE_ALIGNMENT, IndexBufferRef->ResourceLocation);
    
    IndexBufferRef->CreateDerivedViews(Size, Format);
    return IndexBufferRef;
}

TD3D12ConstantBufferRef TD3D12RHI::CreateConstantBuffer(const void* Contents, uint32_t Size)
{
    TD3D12ConstantBufferRef ConstantBufferRef = std::make_shared<TD3D12ConstantBuffer>();

    void* MappedData = UploadBufferAllocator->AllocUploadResource(Size, UPLOAD_RESOURCE_ALIGNMENT, ConstantBufferRef->ResourceLocation);
    if(Contents!= nullptr)
        memcpy(MappedData, Contents, Size);

    ConstantBufferRef->CreateDerivedViews(Size);

    return ConstantBufferRef;
}

TD3D12StructuredBufferRef TD3D12RHI::CreateStructuredBuffer(const void* Contents, uint32_t Elementsize, uint32_t ElementCount)
{
    assert(Contents != nullptr && Elementsize > 0 && ElementCount > 0);

    TD3D12StructuredBufferRef StructuredBufferRef = std::make_shared<TD3D12StructuredBuffer>();

    uint32_t DataSize = Elementsize * ElementCount;
    // Align to ElemenSize;
    void* MappedData = UploadBufferAllocator->AllocUploadResource(DataSize, Elementsize, StructuredBufferRef->ResourceLocation);

    memcpy(MappedData, Contents, DataSize);

    StructuredBufferRef->CreateDerivedView(Elementsize, ElementCount);

    return StructuredBufferRef;
}

TD3D12RWStructuredBufferRef TD3D12RHI::CreateRWStructuredBuffer(uint32_t Elementsize, uint32_t ElementCount)
{
    TD3D12RWStructuredBufferRef RWStructuredBufferRef = std::make_shared<TD3D12RWStructuredBuffer>();

    uint32_t DataSize = Elementsize * ElementCount;
    // Align to ElementSize
    CreateDefaultBuffer(DataSize, Elementsize, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, RWStructuredBufferRef->ResourceLocation);

    RWStructuredBufferRef->CreateDerivedViews(Elementsize, ElementCount);

    return RWStructuredBufferRef;
}

void TD3D12RHI::CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, TD3D12ResourceLocation& ResourceLocation)
{
    // create default resource
    D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Size, Flags);

    DefaultBufferAllocator->AllocDefaultResource(ResourceDesc, Alignment, ResourceLocation);
}

void TD3D12RHI::CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
    TD3D12RHI::g_CommandContext.ResetCommandList();

    // create DefaultBuffer resource
    CreateDefaultBuffer(Size, Alignment, D3D12_RESOURCE_FLAG_NONE, ResourceLocation);

    // create UploadBuffer resource
    TD3D12ResourceLocation uploadResourceLocation;
    void* MappedData = UploadBufferAllocator->AllocUploadResource(Size, Alignment, uploadResourceLocation);

    // Copy contents to upload resource
    memcpy(MappedData, Contents, Size);

    // Copy data from upload resource  to default resource
    TD3D12Resource* DefaultBuffer = ResourceLocation.UnderlyingResource;
    TD3D12Resource* UploadBuffer = uploadResourceLocation.UnderlyingResource;

    auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(DefaultBuffer->D3DResource.Get(), DefaultBuffer->CurrentState, D3D12_RESOURCE_STATE_COPY_DEST);

    TD3D12RHI::g_CommandContext.GetCommandList()->ResourceBarrier(1, &barrier1);

    TD3D12RHI::g_CommandContext.GetCommandList()->CopyBufferRegion(DefaultBuffer->D3DResource.Get(), ResourceLocation.OffsetFromBaseOfResource, UploadBuffer->D3DResource.Get(), uploadResourceLocation.OffsetFromBaseOfResource, Size);

    TD3D12RHI::g_CommandContext.ExecuteCommandLists();
}

void TD3D12VertexBuffer::CreateDerivedViews(uint32_t Size, UINT Stride)
{
    // 由于DefaultBufferAllocator共用一个Resource（状态相同）进行手动分化，
    // 所以所以不能直接获取通过GetGPUVirtualAddress()方法来获取Resource的GPUVirtualAddress地址
    //m_VBV.BufferLocation = ResourceLocation.UnderlyingResource->D3DResource->GetGPUVirtualAddress();
    // 上述方法获取的GPU地址，会给出初始的Resource位置，并非划分后的对应的地址
    // 调换VertexBuffer和IndexBuffer可以进行检验

    m_VBV.BufferLocation = ResourceLocation.GPUVirtualAddress;
    m_VBV.SizeInBytes = Size;
    m_VBV.StrideInBytes = Stride;
}

void TD3D12IndexBuffer::CreateDerivedViews(uint32_t Size, DXGI_FORMAT Format)
{
    //m_IBV.BufferLocation = ResourceLocation.UnderlyingResource->D3DResource->GetGPUVirtualAddress();
    
    m_IBV.BufferLocation = ResourceLocation.GPUVirtualAddress;
    m_IBV.SizeInBytes = Size;
    m_IBV.Format = Format;
}

void TD3D12ConstantBuffer::CreateDerivedViews(uint32_t Size)
{
    CBV_Desc.BufferLocation = ResourceLocation.GPUVirtualAddress;
    CBV_Desc.SizeInBytes = (sizeof(Size) + 255) & ~255; // Align to 256 bytes

    auto HeapHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    TD3D12RHI::g_Device->CreateConstantBufferView(&CBV_Desc, HeapHandle);
}

void TD3D12StructuredBuffer::CreateDerivedView(uint32_t Elementsize, uint32_t ElementCount)
{
    TD3D12ResourceLocation& Location = this->ResourceLocation;
    const uint64_t Offset = Location.OffsetFromBaseOfResource;
    ID3D12Resource* BufferResource = Location.UnderlyingResource->D3DResource.Get();

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    SrvDesc.Buffer.StructureByteStride = Elementsize;
    SrvDesc.Buffer.NumElements = ElementCount;
    SrvDesc.Buffer.FirstElement = Offset / Elementsize;

    SRV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    TD3D12RHI::g_Device->CreateShaderResourceView(BufferResource, &SrvDesc, SRV);
}

void TD3D12RWStructuredBuffer::CreateDerivedViews(uint32_t Elementsize, uint32_t ElementCount)
{
    TD3D12ResourceLocation& Location = this->ResourceLocation;
    const uint64_t Offset = Location.OffsetFromBaseOfResource;
    ID3D12Resource* BufferResource = Location.UnderlyingResource->D3DResource.Get();

    D3D12_SHADER_RESOURCE_VIEW_DESC SrvDesc = {};
    SrvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SrvDesc.Format = DXGI_FORMAT_UNKNOWN;
    SrvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    SrvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    SrvDesc.Buffer.StructureByteStride = Elementsize;
    SrvDesc.Buffer.NumElements = ElementCount;
    SrvDesc.Buffer.FirstElement = Offset / Elementsize;

    SRV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    TD3D12RHI::g_Device->CreateShaderResourceView(BufferResource, &SrvDesc, SRV);

    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    UAVDesc.Format = DXGI_FORMAT_UNKNOWN;
    UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    UAVDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;
    UAVDesc.Buffer.StructureByteStride = Elementsize;
    UAVDesc.Buffer.NumElements = ElementCount;
    UAVDesc.Buffer.FirstElement = Offset / Elementsize;
    UAVDesc.Buffer.CounterOffsetInBytes = 0;

    UAV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    TD3D12RHI::g_Device->CreateUnorderedAccessView(BufferResource, nullptr, &UAVDesc, UAV);
}
