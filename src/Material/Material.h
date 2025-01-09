#pragma once
#include <DirectXMath.h>
#include <unordered_map>
#include <string>
#include <d3d12.h>
#include <memory>
#include "D3D12Utils.h"
#include "Shader.h"


enum class EShadingMode
{
	DefaultLit,
	Unlit,
};

struct MaterialParameters
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.0, 1.0, 1.0,1.0 };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01, 0.01, 0.01 };
	float Roughness = 1.0;

	DirectX::XMFLOAT3 EmissvieColor = { 0.0, 0.0, 0.0 };

	// Used in texture mapping
	DirectX::XMFLOAT4X4 MatTransform = MATH::IdentityMatrix;

	std::unordered_map<std::string/*Parameter*/, std::string/*TextureName*/> TextureMap;
};

struct MaterialRenderState
{
	D3D12_CULL_MODE CullMode = D3D12_CULL_MODE_BACK;

	D3D12_COMPARISON_FUNC DepthFun = D3D12_COMPARISON_FUNC_LESS;
};
class Material
{
public:
	Material(const std::string& InName, const std::string& InShaderName);

	TShader* GetShader(const TShaderDefines& ShaderDefines);

public:
	std::string Name;

	EShadingMode ShadingModel = EShadingMode::DefaultLit;

	MaterialParameters Parameters;

	MaterialRenderState RenderState;

private:

	std::string ShaderName;

	std::unordered_map<TShaderDefines, std::unique_ptr<TShader>> ShaderMap;

};

