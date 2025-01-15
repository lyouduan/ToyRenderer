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

		// Cerberus textures
		{
			TD3D12Texture Cerberus_A;
			Cerberus_A.CreateCube(64, 64);
			if (Cerberus_A.CreateWICFromFile(L"./textures/Cerberus_A.jpg", 0, false))
				m_SrvMaps["Cerberus_A"] = Cerberus_A.GetSRV();

			TD3D12Texture Cerberus_M;
			Cerberus_M.CreateCube(64, 64);
			if (Cerberus_M.CreateWICFromFile(L"./textures/Cerberus_M.jpg", 0, false))
				m_SrvMaps["Cerberus_M"] = Cerberus_M.GetSRV();

			TD3D12Texture Cerberus_N;
			Cerberus_N.CreateCube(64, 64);
			if (Cerberus_N.CreateWICFromFile(L"./textures/Cerberus_N.jpg", 0, false))
				m_SrvMaps["Cerberus_N"] = Cerberus_N.GetSRV();

			TD3D12Texture Cerberus_R;
			Cerberus_R.CreateCube(64, 64);
			if (Cerberus_R.CreateWICFromFile(L"./textures/Cerberus_R.jpg", 0, false))
				m_SrvMaps["Cerberus_R"] = Cerberus_R.GetSRV();
		}
		
	}
	void DestroyTexture()
	{
		m_SrvMaps.clear();
	}
};
