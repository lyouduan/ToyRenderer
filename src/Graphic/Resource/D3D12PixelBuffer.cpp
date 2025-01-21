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

    ResourceLocation.UnderlyingResource->D3DResource->SetName(Name.c_str());
}

DXGI_FORMAT D3D12PixelBuffer::GetDepthFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
        // 32-bit Z w/ Stencil
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;

        // No Stencil
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;

        // 24-bit Z
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;

        // 16-bit Z w/o Stencil
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_R16_UNORM;

    default:
        return DXGI_FORMAT_UNKNOWN;
    }
}

DXGI_FORMAT D3D12PixelBuffer::GetUAVFormat(DXGI_FORMAT defaultFormat)
{
    switch (defaultFormat)
    {
    case DXGI_FORMAT_R8G8B8A8_TYPELESS:
    case DXGI_FORMAT_R8G8B8A8_UNORM:
    case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
        return DXGI_FORMAT_R8G8B8A8_UNORM;

    case DXGI_FORMAT_B8G8R8A8_TYPELESS:
    case DXGI_FORMAT_B8G8R8A8_UNORM:
    case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8A8_UNORM;

    case DXGI_FORMAT_B8G8R8X8_TYPELESS:
    case DXGI_FORMAT_B8G8R8X8_UNORM:
    case DXGI_FORMAT_B8G8R8X8_UNORM_SRGB:
        return DXGI_FORMAT_B8G8R8X8_UNORM;

    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_R32_FLOAT;

#ifdef _DEBUG
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
    case DXGI_FORMAT_D16_UNORM:

        assert(false && "Requested a UAV Format for a depth stencil Format.");
#endif

    default:
        return defaultFormat;
    }
}

void D3D12PixelBuffer::CreateTextureResource(D3D12_RESOURCE_STATES State, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    (void)VidMemPtr;

    CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
    Microsoft::WRL::ComPtr<ID3D12Resource> Resource;

    g_Device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, State, &ClearValue, IID_PPV_ARGS(&Resource));
    
    TD3D12Resource* NewResource = new TD3D12Resource(Resource, State);
    ResourceLocation.UnderlyingResource = NewResource;
    ResourceLocation.SetType(TD3D12ResourceLocation::EResourceLocationType::StandAlone);
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
    CreateTextureResource(D3D12_RESOURCE_STATE_COMMON, ResourceDesc, ClearValue, VidMemPtr);
    // create view
    CreateDerivedViews(g_Device, Format, 1, NumMips);

    ResourceLocation.UnderlyingResource->D3DResource->SetName(name.c_str());
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

    CreateTextureResource(D3D12_RESOURCE_STATE_COMMON, ResourceDesc, ClearValue, VidMem);
    CreateDerivedViews(g_Device, Format, ArrayCount, 1);
}

void D3D12ColorBuffer::Destroy()
{
    m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    ResourceLocation.UnderlyingResource->D3DResource = nullptr;
}

void D3D12ColorBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
    m_NumMipMaps = NumMips - 1;

    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

    RTVDesc.Format = Format;
    UAVDesc.Format = GetUAVFormat(Format);
    SRVDesc.Format = Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;


    // TEXTURE2DArray
    if (ArraySize > 1)
    {
        RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
        RTVDesc.Texture2DArray.MipSlice = 0;
        RTVDesc.Texture2DArray.FirstArraySlice = 0;
        RTVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
        UAVDesc.Texture2DArray.MipSlice = 0;
        UAVDesc.Texture2DArray.FirstArraySlice = 0;
        UAVDesc.Texture2DArray.ArraySize = (UINT)ArraySize;

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

        UAVDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        UAVDesc.Texture2D.MipSlice = 0;

        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = NumMips;
        SRVDesc.Texture2D.MostDetailedMip = 0;
    }

    // Create Handle
    if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_RTVHandle = RTVHeapSlotAllocator->AllocateHeapSlot().Handle;
        m_SRVHandle = SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
        m_UAVHandle = SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    }

    ID3D12Resource* Resource = ResourceLocation.UnderlyingResource->D3DResource.Get();

    // create the render target view
    Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

    // create the shader resource view
    Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

    Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle);
}

void D3D12DepthBuffer::Create(const std::wstring& name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    Create(name, Width, Height, 1, Format, VidMemPtr);
}

void D3D12DepthBuffer::Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    ResourceDesc.SampleDesc.Count = NumSamples;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.DepthStencil.Depth = m_ClearDepth;
    ClearValue.DepthStencil.Stencil = m_ClearStencil;

    CreateTextureResource(D3D12_RESOURCE_STATE_COMMON, ResourceDesc, ClearValue, VidMemPtr);
    CreateDerivedViews(g_Device, Format);
}

DXGI_FORMAT D3D12DepthBuffer::GetDSVFormat(DXGI_FORMAT Format)
{
    switch (Format)
    {
        // 32-bit Z w/ Stencil
    case DXGI_FORMAT_R32G8X24_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
    case DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS:
    case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT:
        return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;

        // No Stencil
    case DXGI_FORMAT_R32_TYPELESS:
    case DXGI_FORMAT_D32_FLOAT:
    case DXGI_FORMAT_R32_FLOAT:
        return DXGI_FORMAT_D32_FLOAT;

        // 24-bit Z
    case DXGI_FORMAT_R24G8_TYPELESS:
    case DXGI_FORMAT_D24_UNORM_S8_UINT:
    case DXGI_FORMAT_R24_UNORM_X8_TYPELESS:
    case DXGI_FORMAT_X24_TYPELESS_G8_UINT:
        return DXGI_FORMAT_D24_UNORM_S8_UINT;

        // 16-bit Z w/o Stencil
    case DXGI_FORMAT_R16_TYPELESS:
    case DXGI_FORMAT_D16_UNORM:
    case DXGI_FORMAT_R16_UNORM:
        return DXGI_FORMAT_D16_UNORM;

    default:
        return Format;
    }
}

void D3D12DepthBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format)
{
    ID3D12Resource* Resource = ResourceLocation.UnderlyingResource->D3DResource.Get();

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc;

    DSVDesc.Format = GetDSVFormat(Format);

    if (Resource->GetDesc().SampleDesc.Count == 1)
    {
        DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        DSVDesc.Texture2D.MipSlice = 0;
    }
    // multi-sampling
    else
    {
        DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
    }

    if (m_DSVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_DSVHandle = DSVHeapSlotAllocator->AllocateHeapSlot().Handle;
    }

    DSVDesc.Flags = D3D12_DSV_FLAG_NONE;
    Device->CreateDepthStencilView(Resource, &DSVDesc, m_DSVHandle);

    // create SRV

    if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_SRVHandle = SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = GetDepthFormat(Format);
    if(DSVDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        SRVDesc.Texture2D.MipLevels = 1;
    }
    else
    {
        SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
    }

    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

    Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);
}

void D3D12CubeBuffer::Create(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    NumMips = (NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);
    D3D12_RESOURCE_FLAGS Flags = CombineResourceFlags();

    D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 6, NumMips, Format, Flags);

    ResourceDesc.SampleDesc.Count = m_FragmentCount;
    ResourceDesc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE ClearValue = {};
    ClearValue.Format = Format;
    ClearValue.Color[0] = m_ClearColor.x;
    ClearValue.Color[1] = m_ClearColor.y;
    ClearValue.Color[2] = m_ClearColor.z;
    ClearValue.Color[3] = m_ClearColor.w;

    // create Texture Resource
    CreateTextureResource(D3D12_RESOURCE_STATE_COMMON, ResourceDesc, ClearValue, VidMemPtr);
    // create view
    CreateDerivedViews(g_Device, Format, 6, NumMips);

    ResourceLocation.UnderlyingResource->D3DResource->SetName(name.c_str());
}

void D3D12CubeBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
    m_NumMipMaps = NumMips - 1;

    D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
    SRVDesc.Format = Format;
    SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE; // CubeMap
    SRVDesc.TextureCube.MipLevels = NumMips;
    SRVDesc.TextureCube.ResourceMinLODClamp = 0.0;
    SRVDesc.TextureCube.MostDetailedMip = 0;
    
    if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
    {
        m_SRVHandle = SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
    }

    ID3D12Resource* Resource = ResourceLocation.UnderlyingResource->D3DResource.Get();

    // create the shader resource view
    Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);
    
    D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
    RTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY; // six faces
    RTVDesc.Texture2DArray.ArraySize = 1; 
    RTVDesc.Format = Format;
    for (int mip = 0; mip < NumMips; ++mip)
    {

        RTVDesc.Texture2DArray.MipSlice = mip;
        for (UINT i = 0; i < 6; ++i)
        {
            RTVDesc.Texture2DArray.FirstArraySlice = i;
            if (m_RTVHandle[mip * 6 + i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
                m_RTVHandle[mip * 6 + i] = RTVHeapSlotAllocator->AllocateHeapSlot().Handle;

            Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle[mip * 6 + i]);
        }
    }
}
