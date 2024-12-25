#pragma once
#include "PSO.h"
class TShader;

namespace PSOManager
{
	extern std::unique_ptr<TShader> m_shader;

	extern std::unordered_map<std::string, GraphicsPSO> m_gfxPSOMap;

	void InitializePSO();
};

