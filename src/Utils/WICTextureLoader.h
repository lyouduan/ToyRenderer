#pragma once
#include "stdafx.h"
#include <stdint.h>
#include "D3D12Texture.h"

namespace DirectX
{
	enum WIC_LOADER_FLAGS : uint32_t
	{
		WIC_LOADER_DEFAULT = 0,
		WIC_LOADER_FORCE_SRGB = 0x1,
		WIC_LOADER_IGNORE_SRGB = 0x2,
		WIC_LOADER_SRGB_DEFAULT = 0x4,
		WIC_LOADER_MIP_AUTOGEN = 0x8,
		WIC_LOADER_MIP_RESERVE = 0x10,
		WIC_LOADER_FIT_POW2 = 0x20,
		WIC_LOADER_MAKE_SQUARE = 0x40,
		WIC_LOADER_FORCE_RGBA32 = 0x80,
	};

	HRESULT __cdecl CreateWICTextureFromFile(
		_In_z_ const wchar_t* fileName,
		size_t maxsize,
		D3D12_RESOURCE_FLAGS resFlags,
		WIC_LOADER_FLAGS loadFlags,
		TextureInfo& TextureInfo,
		D3D12_SUBRESOURCE_DATA& initData,
		std::vector<uint8_t>& decodedData);


	HRESULT __cdecl CreateWICTextureFromMemory(
		size_t maxsize,
		D3D12_SUBRESOURCE_DATA& subresource,
		TD3D12Texture& tex);
	
}