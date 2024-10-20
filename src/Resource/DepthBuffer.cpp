#include "DepthBuffer.h"
#include "D3D12RHI.h"
void DepthBuffer::Create(ID3D12Device* Device, const std::wstring& Name, uint32_t Width, uint32_t Height, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	Create(Device, Name, Width, Height, 1, Format, VidMemPtr);
}

void DepthBuffer::Create(ID3D12Device* Device, const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumSamples, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr)
{
	D3D12_RESOURCE_DESC ResourceDesc = DescribeTex2D(Width, Height, 1, 1, Format, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

	ResourceDesc.SampleDesc.Count = NumSamples;

	D3D12_CLEAR_VALUE ClearValue = {};
	ClearValue.Format = Format;

	CreateTextureResource(Device, Name, ResourceDesc, ClearValue, VidMemPtr);
	CreateDerivedViews(Device, Format);
}

void DepthBuffer::CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format)
{
	ID3D12Resource* Resource = m_pResource.Get();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	dsvDesc.Format = Format;
	if (Resource->GetDesc().SampleDesc.Count == 1)
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Texture2D.MipSlice = 0;
	}
	else
	{
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
	}

	if (m_hDSV[0].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN) 
	{
		m_hDSV[0] = TD3D12RHI::DSVHeapSlotAllocator->AllocateHeapSlot().Handle;
		m_hDSV[1] = TD3D12RHI::DSVHeapSlotAllocator->AllocateHeapSlot().Handle;
	}

	dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[0]);

	dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH;
	Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[1]);

	DXGI_FORMAT stencilReadFormat = Format;
	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (m_hDSV[2].ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		{
			m_hDSV[2] = TD3D12RHI::DSVHeapSlotAllocator->AllocateHeapSlot().Handle;
			m_hDSV[3] = TD3D12RHI::DSVHeapSlotAllocator->AllocateHeapSlot().Handle;
		}

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[2]);

		dsvDesc.Flags = D3D12_DSV_FLAG_READ_ONLY_DEPTH | D3D12_DSV_FLAG_READ_ONLY_STENCIL;
		Device->CreateDepthStencilView(Resource, &dsvDesc, m_hDSV[3]);
	}
	else
	{
		m_hDSV[2] = m_hDSV[0];
		m_hDSV[3] = m_hDSV[1];
	}

	if (m_hDepthSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
		m_hDepthSRV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

	// Create the shader resource view
	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = Format;

	if (dsvDesc.ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D)
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
	}
	else
	{
		SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
	}
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	Device->CreateShaderResourceView(Resource, &SRVDesc, m_hDepthSRV);

	if (stencilReadFormat != DXGI_FORMAT_UNKNOWN)
	{
		if (m_hStencilSRV.ptr == D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN)
			m_hStencilSRV = TD3D12RHI::SRVHeapSlotAllocator->AllocateHeapSlot().Handle;

		SRVDesc.Format = stencilReadFormat;
		SRVDesc.Texture2D.PlaneSlice = 1;

		Device->CreateShaderResourceView(Resource, &SRVDesc, m_hStencilSRV);
	}
}
