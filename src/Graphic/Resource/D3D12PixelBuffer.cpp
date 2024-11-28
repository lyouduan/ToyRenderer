#include "D3D12PixelBuffer.h"
#include "D3D12RHI.h"
#include <iostream>

using namespace TD3D12RHI;

D3D12_RESOURCE_DESC D3D12PixelBuffer::DescribeTex2D(uint32_t Width, uint32_t Height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT Format, UINT Flags)
{
    m_Width = Width;
    m_Height = Height;
    m_ArraySize = DepthOrArraySize;
    m_Format = Format;

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Alignment = 0;
    Desc.DepthOrArraySize = (UINT16)DepthOrArraySize;
    Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    Desc.Flags = (D3D12_RESOURCE_FLAGS)Flags;
    Desc.Format = Format;
    Desc.Height = (UINT)Height;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.MipLevels = (UINT16)NumMips;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Width = (UINT64)Width;
    return Desc;
}

void D3D12PixelBuffer::AssociateWithResource(ID3D12Device* Device, const std::wstring& Name, ID3D12Resource* Resource, D3D12_RESOURCE_STATES CurrentState)
{
    assert(Resource != nullptr);
    D3D12_RESOURCE_DESC ResourceDesc = Resource->GetDesc();

    ResourceLocation.UnderlyingResource = new TD3D12Resource(Resource, CurrentState);

    m_Width = (uint32_t)ResourceDesc.Width; // We don't care about large virtual 
    m_Height = ResourceDesc.Height;
    m_ArraySize = ResourceDesc.DepthOrArraySize;
    m_Format = ResourceDesc.Format;
}

void D3D12PixelBuffer::CreateTextureResource(ID3D12Device* Device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    (void)VidMemPtr;

    TD3D12RHI::TextureResourceAllocator->AllocTextureResource(D3D12_RESOURCE_STATE_PRESENT, ResourceDesc, DEFAULT_RESOURCE_ALIGNMENT, ResourceLocation);

}

void D3D12ColorBuffer::CreateFromSwapChain(const std::wstring& name, ID3D12Resource* BaseResource)
{
    AssociateWithResource(g_Device, name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

    // create RTV
    m_RTVHandle = RTVHeapSlotAllocator->AllocateHeapSlot().Handle;
    g_Device->CreateRenderTargetView(ResourceLocation.UnderlyingResource->D3DResource.Get(), nullptr, m_RTVHandle);
}

void D3D12ColorBuffer::Create(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();

    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, NumMips, Format, Flags);

    ResourceDesc.SampleDesc.Count = m_FragmentCount;
    ResourceDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_ClearColor.x;
    ClearValue.Color[1] = m_ClearColor.y;
    ClearValue.Color[2] = m_ClearColor.z;
    ClearValue.Color[3] = m_ClearColor.w;

    // create Texture Resource
    CreateTextureResource(g_Device, name, ResourceDesc, ClearValue, VidMemPtr);
    // create view
    CreateDerivedViews(g_Device, Format, 1, NumMips);
}

void D3D12ColorBuffer::CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
    DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMem)
{
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();
    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, Flags);

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_ClearColor.x;
    ClearValue.Color[1] = m_ClearColor.y;
    ClearValue.Color[2] = m_ClearColor.z;
    ClearValue.Color[3] = m_ClearColor.w;

    CreateTextureResource(g_Device, Name, ResourceDesc, ClearValue, VidMem);
    CreateDerivedViews(g_Device, Format, ArrayCount, 1);
}

void D3D12ColorBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
    m_NumMipMaps = NumMips - 1;

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

    RTVDesc.Format = Format;
    SRVDesc.Format = Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    // TEXTURE2DArray
    if (ArraySize > 1)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        RTVDesc.Texture2DArray.MipSlice = 0;
        RTVDesc.Texture2DArray.FirstArraySlice = 0;
        RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
        SRVDesc.Texture2DArray.MipLevels = NumMips;
        SRVDesc.Texture2DArray.MostDetailedMip = 0;
        SRVDesc.Texture2DArray.FirstArraySlice = 0;
        SRVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;
    }
    // multi-sampling
    else if (m_FragmentCount > 1)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }
    else
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        RTVDesc.Texture2D.MipSlice = 0;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = NumMips;
        SRVDesc.Texture2D.MostDetailedMip = 0;
    }

    // Create Handle
    if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_RTVHandle = RTVHeapSlotAllocator->AllocateHeapSlot().Handle;
        m_SRVHandle = SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    }

    ID3D12Resource* Resource = ResourceLocation.UnderlyingResource->D3DResource.Get();

    // create the render target view
    Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

    // create the shader resource view
    Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);
}
