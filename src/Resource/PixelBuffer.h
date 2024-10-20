#pragma once
#include "GpuResource.h"

class PixelBuffer : public GpuResource
{
public:
	PixelBuffer() : m_Width(0), m_Height(0), m_ArraySize(0), m_Format(DXGI_FORMAT_UNKNOWN)
	{
	}

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	uint32_t GetDepth() const { return m_ArraySize; }
	const DXGI_FORMAT& GetFormat() const { return m_Format; }

protected:
	D3D12_RESOURCE_DESC DescribeTex2D(uint32_t width, uint32_t height, uint32_t DepthOrArraySize, uint32_t NumMips, DXGI_FORMAT format, UINT Flags);

	void AssociateWithResource(ID3D12Device* device, const std::wstring& Name, ID3D12Resource* resource, D3D12_RESOURCE_STATES currentState);

	void CreateTextureResource(ID3D12Device* device, const std::wstring& Name, const D3D12_RESOURCE_DESC& ResourceDesc, D3D12_CLEAR_VALUE ClearValue, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

protected:

	// buffer info
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_ArraySize;
	DXGI_FORMAT m_Format;
};

