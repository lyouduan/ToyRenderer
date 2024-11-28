#pragma once
#include "D3D12Resource.h"
#include <memory>

class TD3D12Texture
{
public:
	TD3D12Texture(size_t Width, size_t Height, DXGI_FORMAT Format);

	void Create2D();

	bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
	bool CreateDDSFromFile(const wchar_t* fileName, size_t fileSize, bool sRGB);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_hCpuDescriptorHandle; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	uint32_t GetDepth() const { return m_Depth; }

	TD3D12Resource* GetResource() { return ResourceLocation.UnderlyingResource; }

	ID3D12Resource* GetD3DResource() { return ResourceLocation.UnderlyingResource->D3DResource.Get(); }

public:
	TD3D12ResourceLocation ResourceLocation;

private:
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
	DXGI_FORMAT m_Format;

	D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
};
