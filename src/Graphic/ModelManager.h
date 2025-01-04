#pragma once
#include "ModelLoader.h"

namespace ModelManager
{
	extern std::unordered_map<std::string, ModelLoader> m_ModelMaps;
	extern std::unordered_map<std::string, Mesh> m_MeshMaps;

	void LoadModel();

	void LoadMesh();

	void DestroyModel();

};

