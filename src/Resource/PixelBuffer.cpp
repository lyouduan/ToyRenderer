#include "PixelBuffer.h"
#include "DXSamplerHelper.h"

D3D12_RESOURCE_DESC PixelBuffer::DescribeTex2D(uint32_t width, uint32_t height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT format, UINT Flags)
{
    m_Width = width;
    m_Height = height;
    m_ArraySize = DepthOrArraySize;
    m_Format = format;

    D3D12_RESOURCE_DESC Desc = {};
    Desc.Alignment = 0;
    Desc.DepthOrArraySize = (UINT16)DepthOrArraySize;
    Desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    Desc.Flags = (D3D12_RESOURCE_FLAGS)Flags;
    Desc.Format = format;
    Desc.Height = (UINT)height;
    Desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    Desc.MipLevels = (UINT16)NumMips;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.Width = (UINT)width;

    return Desc;
}

void PixelBuffer::AssociateWithResource(ID3D12Device* device, const std::wstring& Name, ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState)
{
    assert(resource != nullptr);

    D3D12_RESOURCE_DESC ResourceDesc = resource->GetDesc();

    m_pResource.Attach(resource);
    m_UsageState = currentState;

    m_Width = (uint32_t)ResourceDesc.Width;
    m_Height = ResourceDesc.Height;
    m_ArraySize = ResourceDesc.DepthOrArraySize;
    m_Format = ResourceDesc.Format;
}

void PixelBuffer::CreateTextureResource(ID3D12Device* device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
    Destroy();

    {
        CD3DX12_HEAP_PROPERTIES HeapProps(D3D12_HEAP_TYPE_DEFAULT);
        ThrowIfFailed(device->CreateCommittedResource(&HeapProps, D3D12_HEAP_FLAG_NONE, &ResourceDesc, D3D12_RESOURCE_STATE_COMMON, &ClearValue, IID_PPV_ARGS(&m_pResource)));
    }

    m_UsageState = D3D12_RESOURCE_STATE_COMMON;
    m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
}
