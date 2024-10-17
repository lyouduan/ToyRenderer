#include "D3D12Buffer.h"
#include "stdafx.h"
namespace TD3D12RHI
{
    std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator = nullptr;

    std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator = nullptr;

    void InitialzeAllocator(ID3D12Device* Device)
    {
        UploadBufferAllocator = std::make_unique<TD3D12UploadBufferAllocator>(Device);
        DefaultBufferAllocator = std::make_unique<TD3D12DefaultBufferAllocator>(Device);
    }
}



TD3D12VertexBufferRef TD3D12RHI::CreateVertexBuffer(const void* Contents, uint32_t Size)
{
    TD3D12VertexBufferRef VertexBufferRef = std::make_shared<TD3D12VertexBuffer>();



    return TD3D12VertexBufferRef();
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

TD3D12ResourceLocation TD3D12RHI::CreateAndInitDefaultBuffer(const void* Contents, uint32_t Size, uint32_t Alignment, TD3D12ResourceLocation& ResourceLocation)
{
    // create DefaultBuffer resource
    CreateDefaultBuffer(Size, Alignment,D3D12_RESOURCE_FLAG_NONE, ResourceLocation);

    // create UploadBuffer resource
    TD3D12ResourceLocation uploadResourceLocation;
    void* MappedData = UploadBufferAllocator->AllocUploadResource(Size, Alignment, uploadResourceLocation);

    // Copy contents to upload resource
    memcpy(MappedData, Contents, Size);


    // Copy data from upload resource  to default resource
    //TD3D12Resource* DefaultBuffer = ResourceLocation.UnderlyingResource;
    //TD3D12Resource* UploadBuffer = uploadResourceLocation.UnderlyingResource;
    return uploadResourceLocation;
}
