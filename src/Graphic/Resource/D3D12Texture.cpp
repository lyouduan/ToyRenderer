#include "D3D12Texture.h"
#include "D3D12RHI.h"
#include "DDSTextureLoader.h"

#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

TD3D12Texture::TD3D12Texture(size_t Width, size_t Height, DXGI_FORMAT Format)
{
	m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    m_Width = (uint32_t)Width;
    m_Height = (uint32_t)Height;
    m_Format = Format;
    m_Depth = 1;

    Create2D();
}

void TD3D12Texture::Create2D()
{
	auto state = D3D12_RESOURCE_STATE_COPY_DEST; // 

    D3D12_RESOURCE_DESC texDesc = {};
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Width = m_Width;
    texDesc.Height = (UINT)m_Height;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = m_Format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    TD3D12RHI::TextureResourceAllocator->AllocTextureResource(state, texDesc, DEFAULT_RESOURCE_ALIGNMENT, ResourceLocation);

    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    TD3D12RHI::g_Device->CreateShaderResourceView(ResourceLocation.UnderlyingResource->D3DResource.Get(), nullptr, m_hCpuDescriptorHandle);
}

bool TD3D12Texture::CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB)
{
    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    HRESULT hr = CreateDDSTextureFromMemory(TD3D12RHI::g_Device,
        (const uint8_t*)memBuffer, fileSize, 0, sRGB, &ResourceLocation.UnderlyingResource->D3DResource, m_hCpuDescriptorHandle);
    
    return SUCCEEDED(hr);
}

bool TD3D12Texture::CreateDDSFromFile(const wchar_t* fileName, size_t fileSize, bool sRGB)
{
    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    HRESULT hr = CreateDDSTextureFromFile(TD3D12RHI::g_Device, fileName, ResourceLocation.UnderlyingResource->D3DResource.GetAddressOf(), m_hCpuDescriptorHandle, fileSize, sRGB);

    return SUCCEEDED(hr);
}

void TD3D12RHI::InitializeTexture(TD3D12Resource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.D3DResource.Get(), 0, NumSubresources);

    TD3D12RHI::g_CommandContext.ResetCommandAllocator();
    TD3D12RHI::g_CommandContext.ResetCommandList();

    // create UploadBuffer resource
    TD3D12ResourceLocation uploadResourceLocation;
    UploadBufferAllocator->AllocUploadResource(uploadBufferSize, 256, uploadResourceLocation);
    TD3D12Resource* UploadBuffer = uploadResourceLocation.UnderlyingResource;

    UpdateSubresources(g_CommandContext.GetCommandList(), Dest.D3DResource.Get(), UploadBuffer->D3DResource.Get(), 0, 0, NumSubresources, SubData);

    auto barrier1 = CD3DX12_RESOURCE_BARRIER::Transition(Dest.D3DResource.Get(), Dest.CurrentState, D3D12_RESOURCE_STATE_GENERIC_READ);

    TD3D12RHI::g_CommandContext.GetCommandList()->ResourceBarrier(1, &barrier1);

    TD3D12RHI::g_CommandContext.ExecuteCommandLists();
}
