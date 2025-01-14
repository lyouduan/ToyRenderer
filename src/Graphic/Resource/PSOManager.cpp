#include "PSOManager.h"
#include "D3D12RHI.h"
#include "Shader.h"

using namespace TD3D12RHI;

namespace PSOManager
{
	std::unordered_map<std::string, TShader> m_shaderMap;

	std::unordered_map<std::string, GraphicsPSO> m_gfxPSOMap;

	void InitializePSO()
	{
		{
			TShaderInfo info;
			info.FileName = "shaders/modelShader";
			info.bCreateVS = true;
			info.bCreatePS = true;
			info.bCreateCS = false;
			info.VSEntryPoint = "VSMain";
			info.PSEntryPoint = "PSMain";
			TShader shader(info);
			m_shaderMap["modelShader"] = shader;

			TShaderInfo boxInfo;
			boxInfo.FileName = "shaders/skyboxShader";
			boxInfo.bCreateVS = true;
			boxInfo.bCreatePS = true;
			boxInfo.bCreateCS = false;
			boxInfo.VSEntryPoint = "VSMain";
			boxInfo.PSEntryPoint = "PSMain";
			TShader boxShader(boxInfo);
			m_shaderMap["skyboxShader"] = boxShader;

			TShaderInfo quadInfo;
			quadInfo.FileName = "shaders/quadShader";
			quadInfo.bCreateVS = true;
			quadInfo.bCreatePS = true;
			quadInfo.bCreateCS = false;
			quadInfo.VSEntryPoint = "VSMain";
			quadInfo.PSEntryPoint = "PSMain";
			TShader quadShader(quadInfo);
			m_shaderMap["quadShader"] = quadShader;

			TShaderInfo brdfInfo;
			brdfInfo.FileName = "shaders/brdf";
			brdfInfo.bCreateVS = true;
			brdfInfo.bCreatePS = true;
			brdfInfo.bCreateCS = false;
			brdfInfo.VSEntryPoint = "VSMain";
			brdfInfo.PSEntryPoint = "PSMain";
			TShader brdfShader(brdfInfo);
			m_shaderMap["brdfShader"] = brdfShader;

			TShaderInfo cubemapInfo;
			cubemapInfo.FileName = "shaders/cubeMap";
			cubemapInfo.bCreateVS = true;
			cubemapInfo.bCreatePS = true;
			cubemapInfo.bCreateCS = false;
			cubemapInfo.VSEntryPoint = "VSMain";
			cubemapInfo.PSEntryPoint = "PSMain";
			TShader cubemapShader(cubemapInfo);
			m_shaderMap["cubemapShader"] = cubemapShader;

			TShaderInfo pbrInfo;
			pbrInfo.FileName = "shaders/pbr";
			pbrInfo.bCreateVS = true;
			pbrInfo.bCreatePS = true;
			pbrInfo.bCreateCS = false;
			pbrInfo.VSEntryPoint = "VSMain";
			pbrInfo.PSEntryPoint = "PSMain";
			TShader pbrShader(pbrInfo);
			m_shaderMap["pbrShader"] = pbrShader;

			TShaderInfo lightInfo;
			lightInfo.FileName = "shaders/light";
			lightInfo.bCreateVS = true;
			lightInfo.bCreatePS = true;
			lightInfo.bCreateCS = false;
			lightInfo.VSEntryPoint = "VSMain";
			lightInfo.PSEntryPoint = "PSMain";
			TShader lightShader(lightInfo);
			m_shaderMap["lightShader"] = lightShader;

			TShaderInfo irradianceMapInfo;
			irradianceMapInfo.FileName = "shaders/irradianceMap";
			irradianceMapInfo.bCreateVS = true;
			irradianceMapInfo.bCreatePS = true;
			irradianceMapInfo.bCreateCS = false;
			irradianceMapInfo.VSEntryPoint = "VSMain";
			irradianceMapInfo.PSEntryPoint = "PSMain";
			TShader irradianceMapShader(irradianceMapInfo);
			m_shaderMap["irradianceMapShader"] = irradianceMapShader;

			TShaderInfo prefilterMapInfo;
			prefilterMapInfo.FileName = "shaders/prefilterMap";
			prefilterMapInfo.bCreateVS = true;
			prefilterMapInfo.bCreatePS = true;
			prefilterMapInfo.bCreateCS = false;
			prefilterMapInfo.VSEntryPoint = "VSMain";
			prefilterMapInfo.PSEntryPoint = "PSMain";
			TShader prefilterMapShader(prefilterMapInfo);
			m_shaderMap["prefilterMapShader"] = prefilterMapShader;
		}

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		GraphicsPSO pso(L"Normal PSO");
		pso.SetShader(&m_shaderMap["modelShader"]);
		pso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		D3D12_RASTERIZER_DESC rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		pso.SetRasterizerState(rasterizerDesc);
		pso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		D3D12_DEPTH_STENCIL_DESC dsvDesc = {};
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		dsvDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		dsvDesc.StencilEnable = FALSE;
		pso.SetDepthStencilState(dsvDesc);
		pso.SetSampleMask(UINT_MAX);
		pso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		pso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		pso.Finalize();
		m_gfxPSOMap["pso"] = pso;

		GraphicsPSO boxPso(L"skybox PSO");
		boxPso.SetShader(&m_shaderMap["skyboxShader"]);
		boxPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // camere inside skybox
		boxPso.SetRasterizerState(rasterizerDesc);
		boxPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // depth = 1
		boxPso.SetDepthStencilState(dsvDesc);
		boxPso.SetSampleMask(UINT_MAX);
		boxPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		boxPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		boxPso.Finalize();
		m_gfxPSOMap["skyboxPSO"] = boxPso;

		GraphicsPSO quadPso(L"quad PSO");
		quadPso.SetShader(&m_shaderMap["quadShader"]);
		quadPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		quadPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		quadPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE; 
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		quadPso.SetDepthStencilState(dsvDesc);
		quadPso.SetSampleMask(UINT_MAX);
		quadPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		quadPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		quadPso.Finalize();
		m_gfxPSOMap["quadPSO"] = quadPso;

		GraphicsPSO brdfPso(L"brdf PSO");
		brdfPso.SetShader(&m_shaderMap["brdfShader"]);
		brdfPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		brdfPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		brdfPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = FALSE;
		brdfPso.SetDepthStencilState(dsvDesc);
		brdfPso.SetSampleMask(UINT_MAX);
		brdfPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		brdfPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8_UNORM, DXGI_FORMAT_UNKNOWN);
		brdfPso.Finalize();
		m_gfxPSOMap["brdfPSO"] = brdfPso;

		GraphicsPSO pbrPso(L"pbr PSO");
		pbrPso.SetShader(&m_shaderMap["pbrShader"]);
		pbrPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		pbrPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		pbrPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		pbrPso.SetDepthStencilState(dsvDesc);
		pbrPso.SetSampleMask(UINT_MAX);
		pbrPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		pbrPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		pbrPso.Finalize();
		m_gfxPSOMap["pbrPSO"] = pbrPso;

		GraphicsPSO lightPso(L"light PSO");
		lightPso.SetShader(&m_shaderMap["lightShader"]);
		lightPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		lightPso.SetRasterizerState(rasterizerDesc);
		lightPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		lightPso.SetDepthStencilState(dsvDesc);
		lightPso.SetSampleMask(UINT_MAX);
		lightPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		lightPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		lightPso.Finalize();
		m_gfxPSOMap["lightPSO"] = lightPso;

		GraphicsPSO cubemapPso(L"cubemap PSO");
		cubemapPso.SetShader(&m_shaderMap["cubemapShader"]);
		cubemapPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // camere inside skybox
		cubemapPso.SetRasterizerState(rasterizerDesc);
		cubemapPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = FALSE;
		cubemapPso.SetDepthStencilState(dsvDesc);
		cubemapPso.SetSampleMask(UINT_MAX);
		cubemapPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		cubemapPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
		cubemapPso.Finalize();
		m_gfxPSOMap["cubemapPSO"] = cubemapPso;

		GraphicsPSO irradianceMapPso(L"irradianceMap PSO");
		irradianceMapPso.SetShader(&m_shaderMap["irradianceMapShader"]);
		irradianceMapPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // camere inside skybox
		irradianceMapPso.SetRasterizerState(rasterizerDesc);
		irradianceMapPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = FALSE;
		irradianceMapPso.SetDepthStencilState(dsvDesc);
		irradianceMapPso.SetSampleMask(UINT_MAX);
		irradianceMapPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		irradianceMapPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
		irradianceMapPso.Finalize();
		m_gfxPSOMap["irradianceMapPSO"] = irradianceMapPso;

		GraphicsPSO prefilterMapPso(L"prefilterMap PSO");
		prefilterMapPso.SetShader(&m_shaderMap["prefilterMapShader"]);
		prefilterMapPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE; // camere inside skybox
		prefilterMapPso.SetRasterizerState(rasterizerDesc);
		prefilterMapPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = FALSE;
		prefilterMapPso.SetDepthStencilState(dsvDesc);
		prefilterMapPso.SetSampleMask(UINT_MAX);
		prefilterMapPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		prefilterMapPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_UNKNOWN);
		prefilterMapPso.Finalize();
		m_gfxPSOMap["prefilterMapPSO"] = prefilterMapPso;
	}

}
