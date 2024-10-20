#include "ColorBuffer.h"
#include "D3D12RHI.h"

void ColorBuffer::CreateFromSwapChain(ID3D12Device* device, const std::wstring& Name, ID3D12Resource* BaseResource)
{
	AssociateWithResource(device, Name, BaseResource, D3D12_RESOURCE_STATE_PRESENT);

	m_RTVHandle = TD3D12RHI::RTVHeapSlotAllocator->AllocateHeapSlot().Handle;

	device->CreateRenderTargetView(m_pResource.Get(), nullptr, m_RTVHandle);
}

void ColorBuffer::Create(ID3D12Device* device, const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	NumMips = ( NumMips == 0 ? ComputeNumMips(Width, Height) : NumMips);

	D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, NumMips, Format, Flags);

	ResourceDesc.SampleDesc.Count = m_FragmentCount;
	ResourceDesc.SampleDesc.Quality = 0;

	D3D12_CLEAR_VALUE ClearValue = {};

	ClearValue.Format = Format;
	ClearValue.Color[0] = m_ClearColor.x;
	ClearValue.Color[1] = m_ClearColor.y;
	ClearValue.Color[2] = m_ClearColor.z;
	ClearValue.Color[3] = m_ClearColor.w;

	CreateTextureResource(device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(device, Format, 1, NumMips);
}

void ColorBuffer::CreateArray(ID3D12Device* device, const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, ArrayCount, 1, Format, Flags);

	D3D12_CLEAR_VALUE ClearValue = {};

	ClearValue.Format = Format;
	ClearValue.Color[0] = m_ClearColor.x;
	ClearValue.Color[1] = m_ClearColor.y;
	ClearValue.Color[2] = m_ClearColor.z;
	ClearValue.Color[3] = m_ClearColor.w;

	CreateTextureResource(device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(device, Format, ArrayCount, 1);
}

void ColorBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips)
{
	//assert(ArraySize == 1 || NumMips == 1);

	m_NumMipMaps = NumMips - 1;

	D3D12_RENDER_TARGET_VIEW_DESC RTVDesc = {};
	D3D12_UNORDERED_ACCESS_VIEW_DESC UAVDesc = {};
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};

	RTVDesc.Format = Format;
	SRVDesc.Format = Format;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	UAVDesc.Format = Format;
	
	if (ArraySize > 1) // 数组2D纹理
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
	else if (m_FragmentCount > 1) // 多重采样
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

	if (m_SRVHandle.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
	{
		m_RTVHandle = TD3D12RHI::RTVHeapSlotAllocator->AllocateHeapSlot().Handle;
		m_SRVHandle = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;
	}

	ID3D12Resource* Resource = m_pResource.Get();

	// create the render target view
	Device->CreateRenderTargetView(Resource, &RTVDesc, m_RTVHandle);

	// create the shdaer resource view
	Device->CreateShaderResourceView(Resource, &SRVDesc, m_SRVHandle);

	if (m_FragmentCount > 1) return; // 多重采样无法生成mipmap

	// create the UAVs for each mip level (RWTexture2d)

	for (uint32_t i = 0; i < NumMips; ++i)
	{
		if (m_UAVHandle[i].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_UAVHandle[i] = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

		Device->CreateUnorderedAccessView(Resource, nullptr, &UAVDesc, m_UAVHandle[i]);

		UAVDesc.Texture2D.MipSlice++;
	}

}


