#include "D3D12RHI.h"
#include "stdafx.h"
#include "DXSample.h"
#include "D3D12PixelBuffer.h"

using namespace Microsoft::WRL;

namespace TD3D12RHI
{
    ID3D12Device* g_Device = nullptr;
    TD3D12CommandContext g_CommandContext;
    ComPtr<IDXGISwapChain1> g_SwapCHain;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> g_ImGuiSrvHeap;
    D3D12ColorBuffer m_renderTragetrs[FrameCount];

    // buffer
    D3D12DepthBuffer g_DepthBuffer;
    D3D12DepthBuffer g_PreDepthPassBuffer;

    // memory allocator
    std::unique_ptr<TD3D12UploadBufferAllocator> UploadBufferAllocator = nullptr;
    std::unique_ptr<TD3D12DefaultBufferAllocator> DefaultBufferAllocator = nullptr;
    std::unique_ptr<TD3D12TextureResourceAllocator> TextureResourceAllocator = nullptr;
    std::unique_ptr<TD3D12PixelResourceAllocator> PixelResourceAllocator = nullptr;

    // heapSlot allocator
    std::unique_ptr<TD3D12HeapSlotAllocator> RTVHeapSlotAllocator = nullptr;
    std::unique_ptr<TD3D12HeapSlotAllocator> DSVHeapSlotAllocator = nullptr;
    std::unique_ptr<TD3D12HeapSlotAllocator> SRVHeapSlotAllocator = nullptr;
    std::unique_ptr<TD3D12HeapSlotAllocator> ImGuiSRVHeapAllocator = nullptr;

    // cache descriptor handle
    std::unique_ptr<TD3D12DescriptorCache> g_DescriptorCache = nullptr;

    D3D12_CPU_DESCRIPTOR_HANDLE NullDescriptor;

    void Initialze()
    {
        // initialize CommandContext
        g_CommandContext.CreateCommandContext(g_Device);

        InitialzeAllocator();

        InitialzeBuffer();

        g_ImGuiSrvHeap = GetImGuiSRVHeapAllocator();

        NullDescriptor = CreateNullDescriptor();

    }

    void InitialzeBuffer()
    {
        // create the dsv
        g_DepthBuffer.Create(L"Depth Buffer", g_DisplayWidth, g_DisplayHeight, DXGI_FORMAT_D32_FLOAT);
        g_PreDepthPassBuffer.Create(L"pre-Pass Depth Buffer", g_DisplayWidth, g_DisplayHeight, DXGI_FORMAT_D16_UNORM);
    }

    void InitialzeAllocator()
    {
        UploadBufferAllocator = std::make_unique<TD3D12UploadBufferAllocator>(g_Device);
        DefaultBufferAllocator = std::make_unique<TD3D12DefaultBufferAllocator>(g_Device);
        TextureResourceAllocator = std::make_unique<TD3D12TextureResourceAllocator>(g_Device);

        RTVHeapSlotAllocator = std::make_unique<TD3D12HeapSlotAllocator>(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
        DSVHeapSlotAllocator = std::make_unique<TD3D12HeapSlotAllocator>(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 256);
        SRVHeapSlotAllocator = std::make_unique<TD3D12HeapSlotAllocator>(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128);
        ImGuiSRVHeapAllocator = std::make_unique<TD3D12HeapSlotAllocator>(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);

        g_DescriptorCache = std::make_unique<TD3D12DescriptorCache>(g_Device);
    }
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

ComPtr<ID3D12DescriptorHeap> TD3D12RHI::GetImGuiSRVHeapAllocator()
{
    return ImGuiSRVHeapAllocator->AllocateHeapOnly();
}
