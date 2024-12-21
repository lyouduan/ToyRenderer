#pragma once
#include "D3D12Resource.h"
#include <memory>




struct TextureInfo
{
	D3D12_RESOURCE_DIMENSION Dimension;
	size_t Width;
	size_t Height;
	size_t DepthOrArraySize;
	size_t MipCount;
	DXGI_FORMAT Format;

	D3D12_RESOURCE_STATES InitState = D3D12_RESOURCE_STATE_COPY_DEST;

	DXGI_FORMAT SRVFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT RTVFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT DSVFormat = DXGI_FORMAT_UNKNOWN;
	DXGI_FORMAT UAVFormat = DXGI_FORMAT_UNKNOWN;
};

class TD3D12Texture
{
public:
	TD3D12Texture(size_t Width, size_t Height, DXGI_FORMAT Format = DXGI_FORMAT_R16G16B16A16_FLOAT);
	TD3D12Texture();

	void CreateDesc();

	bool CreateDDSFromMemory(const void* memBuffer, size_t fileSize, bool sRGB);
	bool CreateDDSFromFile(const wchar_t* fileName, size_t fileSize, bool sRGB);
	bool CreateWICFromFile(const wchar_t* fileName, size_t fileSize, bool sRGB);
	bool LoadImageFromFile(std::string fileName, size_t fileSize, bool sRGB);

	void Create2D();
	void Create2D(TextureInfo info);

	D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_hCpuDescriptorHandle; }

	uint32_t GetWidth() const { return m_Width; }
	uint32_t GetHeight() const { return m_Height; }
	uint32_t GetDepth() const { return m_Depth; }

	TD3D12Resource* GetResource() { return ResourceLocation.UnderlyingResource; }

	ID3D12Resource* GetD3DResource() { return ResourceLocation.UnderlyingResource->D3DResource.Get(); }

public:
	TD3D12ResourceLocation ResourceLocation;

	std::string name;

private:
	
	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_Depth;
	DXGI_FORMAT m_Format;
	D3D12_RESOURCE_STATES m_state = D3D12_RESOURCE_STATE_COPY_DEST;
	D3D12_RESOURCE_DESC m_Desc;

	TextureInfo m_TexInfo;

	std::vector<D3D12_SUBRESOURCE_DATA> m_InitData;
	std::vector<uint8_t> decodedData;

	D3D12_CPU_DESCRIPTOR_HANDLE m_hCpuDescriptorHandle;
};
