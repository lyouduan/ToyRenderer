#pragma once
#include "D3D12Texture.h"
#include <unordered_map>

namespace TextureManager
{
	extern std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> m_SrvMaps;

	void LoadTexture();

	void DestroyTexture();

};

