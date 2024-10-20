#include "D3D12Buffer.h"
#include "stdafx.h"
#include "D3D12RHI.h"

TD3D12VertexBufferRef TD3D12RHI::CreateVertexBuffer(const void* Contents, uint32_t Size, ID3D12GraphicsCommandList* cmdlist)
{
    TD3D12VertexBufferRef VertexBufferRef = std::make_shared<TD3D12VertexBuffer>();

    CreateAndInitDefaultBuffer(Contents, Size, DEFAULT_RESOURCE_ALIGNMENT, VertexBufferRef->ResourceLocation, cmdlist);

    return VertexBufferRef;
}

TD3D12IndexBufferRef TD3D12RHI::CreateIndexBuffer(const void* Contents, uint32_t Size)
{
    return TD3D12IndexBufferRef();
}

void TD3D12RHI::CreateDefaultBuffer(uint32_t Size, uint32_t Alignment, D3D12_RESOURCE_FLAGS Flags, TD3D12ResourceLocation& ResourceLocation)
{
    // create default resource
    D3D12_RESOURCE_DESC ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(Size, Flags);

    DefaultBufferAllocator->AllocDefaultResource(ResourceDesc, Alignment, ResourceLocation);
}

void TD3D12RHI::CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation, ID3D12GraphicsCommandList* cmdlist)
{
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
    DefaultBuffer->CurrentState = D3D12_RESOURCE_STATE_COPY_DEST;

    cmdlist->ResourceBarrier(1, &barrier1);

    cmdlist->CopyBufferRegion(DefaultBuffer->D3DResource.Get(), ResourceLocation.OffsetFromBaseOfResource, UploadBuffer->D3DResource.Get(), uploadResourceLocation.OffsetFromBaseOfResource, Size);

    cmdlist->Close();
}
