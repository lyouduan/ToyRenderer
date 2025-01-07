#include "Material.h"
#include "Shader.h"

Material::Material(const std::string& InName, const std::string& InShaderName)
    : Name(InName), ShaderName(InShaderName)
{
}

TShader* Material::GetShader(const TShaderDefines& ShaderDefines)
{
    auto Iter = ShaderMap.find(ShaderDefines);

    if (Iter == ShaderMap.end())
    {
        // create new shader
        TShaderInfo ShaderInfo;
        ShaderInfo.ShaderName = ShaderName;
        ShaderInfo.FileName = ShaderName;
        ShaderInfo.ShaderDefines = ShaderDefines;
        ShaderInfo.bCreateVS = true;
        ShaderInfo.bCreatePS = true;

        std::unique_ptr<TShader> NewShader = std::make_unique<TShader>(ShaderInfo);

        ShaderMap.insert({ ShaderDefines, std::move(NewShader) });

        return ShaderMap[ShaderDefines].get();
    }
    else
    {
        return Iter->second.get();
    }
}
