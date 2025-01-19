#pragma once
#include "PSO.h"
#include "Shader.h"

namespace PSOManager
{
	extern std::unordered_map<std::string, TShader> m_shaderMap;

	extern std::unordered_map<std::string, GraphicsPSO> m_gfxPSOMap;
	extern std::unordered_map<std::string, ComputePSO> m_ComputePSOMap;

	void InitializeShader();
	void InitializeComputeShader();

	void InitializePSO();
};

