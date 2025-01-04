#include "TextureManager.h"

namespace TextureManager
{
	std::unordered_map<std::string, D3D12_CPU_DESCRIPTOR_HANDLE> m_SrvMaps;

	void LoadTexture()
	{
		TD3D12Texture tex;
		tex.Create2D(64, 64);
		if(tex.CreateDDSFromFile(L"./textures/Wood.dds", 0, false))
			m_SrvMaps["wood"] = tex.GetSRV();

		TD3D12Texture lofttex;
		lofttex.Create2D(64, 64);
		if (lofttex.CreateDDSFromFile(L"./textures/newport_loft.dds", 0, false))
			m_SrvMaps["loft"] = lofttex.GetSRV();

		TD3D12Texture skytex;
		skytex.CreateCube(64, 64);
		if(skytex.CreateDDSFromFile(L"./textures/cubeMap.dds", 0, false))
			m_SrvMaps["skybox"] = skytex.GetSRV();
	}
	void DestroyTexture()
	{
		m_SrvMaps.clear();
	}
};
