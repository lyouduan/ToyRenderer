#pragma once
#include "stdafx.h"
#include "D3D12Resource.h"
#include "D3D12RHI.h"

#include <unordered_map>

enum class EShaderType
{
	VERTEX_SHADER,
	PIXEL_SHADER,
	COMPUTE_SHADER,
};

struct TShaderDefines
{
public:
	void GetD3DShaderMacro(std::vector<D3D_SHADER_MACRO>& outMacros) const;

	bool operator==(const TShaderDefines& Other) const;

	void SetDefine(const std::string& Name, const std::string& Definition);

public:
	std::unordered_map<std::string, std::string> DefinesMap;
};

// declare hash<TShaderDefines>
namespace std
{
	template<>
	struct hash<TShaderDefines>
	{
		std::size_t operator()(const TShaderDefines& Defines) const
		{
			using std::size_t;
			using std::hash;
			using std::string;

			// Compute individual hash values for each string
			// and combine them using XOR and bit shifting.
			size_t HashValue = 0;

			for (const auto& pair : Defines.DefinesMap)
			{
				HashValue ^= (hash<string>()(pair.first) << 1);
				HashValue ^= (hash<string>()(pair.second) << 1);
			}

			return HashValue;
		}
	};
}

struct TShaderParameter
{
	std::string Name;
	EShaderType ShaderType;
	UINT BindPoint;
	UINT RegisterSpace;
};

struct TShaderCBVParameter : TShaderParameter
{
	TD3D12ConstantBufferRef ConstantBufferRef;
};

struct TShaderSRVParameter : TShaderParameter
{
	UINT BindCount;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> SRVList;
};

struct TShaderUAVParameter : TShaderParameter
{
	UINT BindCount;

	std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> UAVList;
};

struct TShaderSamplerParameter : TShaderParameter
{

};

struct TShaderInfo
{
	std::string ShaderName;

	std::string FileName;

	TShaderDefines ShaderDefines;

	bool bCreateVS = false;

	std::string VSEntryPoint = "VS";

	bool bCreatePS = false;

	std::string PSEntryPoint = "PS";

	bool bCreateCS = false;

	std::string CSEntryPoint = "CS";
};

class TShader
{
public:
	TShader() {}
	TShader(const TShaderInfo& InShaderInfo);

	void Initialize();

	//bool SetDescriptorCache(std::shared_ptr<TD3D12DescriptorCache> InDescriptorCache);

	bool SetParameter(std::string ParamName, TD3D12ConstantBufferRef ConstantBufferRef);

	bool SetParameter(std::string ParamName, D3D12_CPU_DESCRIPTOR_HANDLE& SRVHandle);
	bool SetParameter(std::string ParamName, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& SRVHandleList);
	// UAV
	// ...
	bool SetParameterUAV(std::string ParamName, D3D12_CPU_DESCRIPTOR_HANDLE& UAVHandle);

	bool SetParameterUAV(std::string ParamName, std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& UAVHandleList);

	void BindParameters();

private:

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(const std::wstring& FileName, const D3D_SHADER_MACRO* Defines, const std::string& Entrypoint, const std::string& Target);

	void GetShaderParameter(Microsoft::WRL::ComPtr<ID3DBlob> PassBlob, EShaderType ShaderType);

	D3D12_SHADER_VISIBILITY GetShaderVisibility(EShaderType ShaderType);

	std::vector<CD3DX12_STATIC_SAMPLER_DESC> CreateStaticSamplers();

	void CreateRootSignature();

	void CheckBindings();

	void ClearBindings();

public:

	TShaderInfo ShaderInfo;

	std::vector<TShaderCBVParameter> CBVParams;

	std::vector<TShaderSRVParameter> SRVParams;

	std::vector<TShaderUAVParameter> UAVParams;

	std::vector<TShaderSamplerParameter> SamplerParams;

	//std::vector<TShaderUAVParameter> UAVParams;


	int CBVSignatureBaseBindSlot = -1;

	int SRVSignatureBindSlot = -1;

	UINT SRVCount = 0;

	int UAVSignatureBindSlot = -1;

	UINT UAVCount = 0;

	int SamplerSignatureBindSlot = -1;

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> ShaderPass;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;

	//std::shared_ptr<TD3D12DescriptorCache> descriptorCache = nullptr;
};