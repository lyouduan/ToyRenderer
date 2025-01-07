#pragma once
#include "Material.h"
#include "D3D12Buffer.h"

class MaterialInstance
{
public:
	MaterialInstance(Material* Parent, const std::string& InName);

public:
	void SetTextureParameter(const std::string& Parameter, const std::string& TextureName);

	void CreateMaterialConstantBuffer();

public:
	
	Material* material = nullptr;

	std::string Name;

	MaterialParameters Parameters;

	TD3D12ConstantBufferRef MaterialConstantBuffer = nullptr;
};

