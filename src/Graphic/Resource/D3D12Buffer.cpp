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

    void* Mappedata = UploadBufferAllocator->AllocUploadResource(Size, UPLOAD_RESOURCE_ALIGNMENT, ConstantBufferRef->ResourceLocation);

    memcpy(Mappedata, Contents, Size);

    ConstantBufferRef->CreateDerivedViews(Size);

    return ConstantBufferRef;
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
