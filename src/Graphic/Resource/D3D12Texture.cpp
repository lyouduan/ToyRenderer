#define STBI_NO_SIMD
#define STBI_NO_BUILTIN
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "D3D12Texture.h"
#include "D3D12RHI.h"
#include "DDSTextureLoader.h"
#include "WICTextureLoader.h"

#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

TD3D12Texture::TD3D12Texture(size_t Width, size_t Height, DXGI_FORMAT Format)
{
	m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    m_Width = (uint32_t)Width;
    m_Height = (uint32_t)Height;
    m_Format = Format;
    m_Depth = 1;

    CreateDesc();
    Create2D();
}

TD3D12Texture::TD3D12Texture()
{
    m_hCpuDescriptorHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
}

void TD3D12Texture::CreateDesc()
{
	m_state = D3D12_RESOURCE_STATE_COPY_DEST; // 

    m_Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    m_Desc.Width = m_Width;
    m_Desc.Height = (UINT)m_Height;
    m_Desc.Alignment = 0;
    m_Desc.DepthOrArraySize = 1;
    m_Desc.MipLevels = 1;
    m_Desc.Format = m_Format;
    m_Desc.SampleDesc.Count = 1;
    m_Desc.SampleDesc.Quality = 0;
    m_Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    m_Desc.Flags = D3D12_RESOURCE_FLAG_NONE;
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

bool TD3D12Texture::CreateWICFromFile(const wchar_t* fileName, size_t fileSize, bool sRGB)
{
    D3D12_SUBRESOURCE_DATA InitData;

    DirectX::WIC_LOADER_FLAGS LoadFlags;
    if (sRGB)
    {
        LoadFlags = DirectX::WIC_LOADER_FORCE_SRGB;
    }
    else
    {
        LoadFlags = DirectX::WIC_LOADER_IGNORE_SRGB;
    }

    HRESULT hr = DirectX::CreateWICTextureFromFile(fileName, 0u, D3D12_RESOURCE_FLAG_NONE, LoadFlags,
        m_TexInfo, InitData, decodedData);

    Create2D(m_TexInfo);

    m_InitData.push_back(InitData);

    TD3D12Resource DestTexture(this->GetD3DResource(), D3D12_RESOURCE_STATE_COPY_DEST);

    TD3D12RHI::UploadTextureData(DestTexture, m_InitData);

    return SUCCEEDED(hr);
}

void TD3D12Texture::Create2D()
{
    if (m_Width != m_Desc.Width)
    {
        m_Width = m_Desc.Width;
        m_Height = m_Desc.Height;
        m_Format = m_Desc.Format;
        m_Depth = m_Desc.DepthOrArraySize;

    }

    TD3D12RHI::TextureResourceAllocator->AllocTextureResource(m_state, m_Desc, DEFAULT_RESOURCE_ALIGNMENT, ResourceLocation);

    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    TD3D12RHI::g_Device->CreateShaderResourceView(ResourceLocation.UnderlyingResource->D3DResource.Get(), nullptr, m_hCpuDescriptorHandle);
}

void TD3D12Texture::Create2D(TextureInfo info)
{
    m_state = info.InitState; // 

    m_Desc.Dimension = info.Dimension;
    m_Desc.Width = info.Width;
    m_Desc.Height = info.Height;
    m_Desc.Alignment = 0;
    m_Desc.DepthOrArraySize = info.DepthOrArraySize;
    m_Desc.MipLevels = info.MipCount;
    m_Desc.Format = info.Format;
    m_Desc.SampleDesc.Count = 1;
    m_Desc.SampleDesc.Quality = 0;
    m_Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    m_Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    TD3D12RHI::TextureResourceAllocator->AllocTextureResource(m_state, m_Desc, 256, ResourceLocation);

    if (m_hCpuDescriptorHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
        m_hCpuDescriptorHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    SRVDesc.Texture2D.MostDetailedMip = 0;
    SRVDesc.Texture2D.MipLevels = 1;
    SRVDesc.Texture2D.PlaneSlice = 0;
    SRVDesc.Texture2D.ResourceMinLODClamp = 0.0;
    SRVDesc.Format = info.Format;

    TD3D12RHI::g_Device->CreateShaderResourceView(ResourceLocation.UnderlyingResource->D3DResource.Get(), &SRVDesc, m_hCpuDescriptorHandle);
}

void TD3D12RHI::InitializeTexture(TD3D12Resource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[])
{
    UINT64 uploadBufferSize = GetRequiredIntermediateSize(Dest.D3DResource.Get(), 0, NumSubresources);

    TD3D12RHI::g_CommandContext.ResetCommandList();

    // create UploadBuffer resource
    TD3D12ResourceLocation uploadResourceLocation;
    UploadBufferAllocator->AllocUploadResource(uploadBufferSize, 256, uploadResourceLocation);
    TD3D12Resource* UploadBuffer = uploadResourceLocation.UnderlyingResource;

    UpdateSubresources(g_CommandContext.GetCommandList(), Dest.D3DResource.Get(), UploadBuffer->D3DResource.Get(), 0, 0, NumSubresources, SubData);

    TD3D12RHI::g_CommandContext.Transition(&Dest, D3D12_RESOURCE_STATE_GENERIC_READ);

    TD3D12RHI::g_CommandContext.ExecuteCommandLists();
}

void TD3D12RHI::UploadTextureData(TD3D12Resource TextureResource, const std::vector<D3D12_SUBRESOURCE_DATA>& InitData)
{
    TD3D12RHI::g_CommandContext.ResetCommandList();

    D3D12_RESOURCE_DESC TexDesc = TextureResource.D3DResource->GetDesc();

    //GetCopyableFootprints
    const UINT NumSubresources = (UINT)InitData.size();
    std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Layouts(NumSubresources);
    std::vector<uint32_t> NumRows(NumSubresources);
    std::vector<uint64_t> RowSizesInBytes(NumSubresources);

    uint64_t RequiredSize = 0;
    TD3D12RHI::g_Device->GetCopyableFootprints(&TexDesc, 0, NumSubresources, 0, &Layouts[0], &NumRows[0], &RowSizesInBytes[0], &RequiredSize);

    //Create upload resource
    TD3D12ResourceLocation UploadResourceLocation;
    void* MappedData = TD3D12RHI::UploadBufferAllocator->AllocUploadResource((uint32_t)RequiredSize, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT, UploadResourceLocation);
    ID3D12Resource* UploadBuffer = UploadResourceLocation.UnderlyingResource->D3DResource.Get();

    //Copy contents to upload resource
    for (uint32_t i = 0; i < NumSubresources; ++i)
    {
        if (RowSizesInBytes[i] > SIZE_T(-1))
        {
            assert(0);
        }
        D3D12_MEMCPY_DEST DestData = { (BYTE*)MappedData + Layouts[i].Offset, Layouts[i].Footprint.RowPitch, SIZE_T(Layouts[i].Footprint.RowPitch) * SIZE_T(NumRows[i]) };
        MemcpySubresource(&DestData, &(InitData[i]), static_cast<SIZE_T>(RowSizesInBytes[i]), NumRows[i], Layouts[i].Footprint.Depth);
    }

    //Copy data from upload resource to default resource
    if(TextureResource.CurrentState != D3D12_RESOURCE_STATE_COPY_DEST)
        TD3D12RHI::g_CommandContext.Transition(&TextureResource, D3D12_RESOURCE_STATE_COPY_DEST);

    for (UINT i = 0; i < NumSubresources; ++i)
    {
        Layouts[i].Offset += UploadResourceLocation.OffsetFromBaseOfResource;

        CD3DX12_TEXTURE_COPY_LOCATION Src;
        Src.pResource = UploadBuffer;
        Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
        Src.PlacedFootprint = Layouts[i];

        CD3DX12_TEXTURE_COPY_LOCATION Dst;
        Dst.pResource = TextureResource.D3DResource.Get();
        Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        Dst.SubresourceIndex = i;

        TD3D12RHI::g_CommandContext.GetCommandList()->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
    }

    TD3D12RHI::g_CommandContext.Transition(&TextureResource, D3D12_RESOURCE_STATE_GENERIC_READ);

    TD3D12RHI::g_CommandContext.ExecuteCommandLists();
}