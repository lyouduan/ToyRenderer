#pragma once
#include "ModelLoader.h"

namespace ModelManager
{
	extern std::unordered_map<std::string, ModelLoader> m_ModelMaps;

	void LoadModel();

	void DestroyModel();

};

