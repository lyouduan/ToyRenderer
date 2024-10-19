#include "D3D12RHI.h"
#include "stdafx.h"

namespace TD3D12RHI
{
    // memory allocator
    std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator = nullptr;
    std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator = nullptr;

    // heapSlot allocator
    std::unique_ptr<TD3D12HeapSlotAllocator> RTVHeapSlotAllocator = nullptr;
    std::unique_ptr<TD3D12HeapSlotAllocator> DSVHeapSlotAllocator = nullptr;
    std::unique_ptr<TD3D12HeapSlotAllocator> SRVHeapSlotAllocator = nullptr;

    // cache descriptor handle
    std::unique_ptr<TD3D12DescriptorCache> DescriptorCache = nullptr;

    void InitialzeAllocator(ID3D12Device* Device)
    {
        UploadBufferAllocator = std::make_unique<TD3D12UploadBufferAllocator>(Device);
        DefaultBufferAllocator = std::make_unique<TD3D12DefaultBufferAllocator>(Device);

        RTVHeapSlotAllocator = std::make_unique<TD3D12HeapSlotAllocator>(Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 128);
        DSVHeapSlotAllocator = std::make_unique<TD3D12HeapSlotAllocator>(Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 128);
        SRVHeapSlotAllocator = std::make_unique<TD3D12HeapSlotAllocator>(Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128);

        DescriptorCache = std::make_unique<TD3D12DescriptorCache>(Device);
    }
}

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

TD3D12HeapSlotAllocator* TD3D12RHI::GetHeapSlotAllocator(D3D12_DESCRIPTOR_HEAP_TYPE Type)
{
    switch (Type)
    {
    case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
    {
        return SRVHeapSlotAllocator.get();
        break;
    }
        
    //case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
    //    break;
    case D3D12_DESCRIPTOR_HEAP_TYPE_RTV:
    {
        return RTVHeapSlotAllocator.get();
        break;
    }

    case D3D12_DESCRIPTOR_HEAP_TYPE_DSV:
    {
        return DSVHeapSlotAllocator.get();
        break;
    }

    default:
        return nullptr;
    }
}
