#include "MaterialInstance.h"
#include "RenderInfo.h"
#include "D3D12RHI.h"

MaterialInstance::MaterialInstance(Material* Parent, const std::string& InName)
	: material(Parent), Name(InName)
{
	Parameters = material->Parameters;
}

void MaterialInstance::SetTextureParameter(const std::string& Parameter, const std::string& TextureName)
{
	auto Iter = Parameters.TextureMap.find(Parameter);

	if (Iter != Parameters.TextureMap.end())
	{
		Iter->second = TextureName;
	}
}

void MaterialInstance::CreateMaterialConstantBuffer()
{

	MatCBuffer matConst;
	matConst.DiffuseAlbedo = Parameters.DiffuseAlbedo;
	matConst.FresnelR0 = Parameters.FresnelR0;
	matConst.Roughness = Parameters.Roughness;
	matConst.MatTransform = Parameters.MatTransform;
	matConst.EmissiveColor = Parameters.EmissvieColor;
	matConst.ShadingModel = (UINT)material->ShadingModel;

	// create ConstantBuffer
	MaterialConstantBuffer = TD3D12RHI::CreateConstantBuffer(&matConst, sizeof(MatCBuffer));
}


