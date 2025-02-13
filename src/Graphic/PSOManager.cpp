#include "PSOManager.h"
#include "D3D12RHI.h"
#include "Shader.h"

using namespace TD3D12RHI;

namespace PSOManager
{
	std::unordered_map<std::string, TShader> m_shaderMap;

	std::unordered_map<std::string, GraphicsPSO> m_gfxPSOMap;
	std::unordered_map<std::string, ComputePSO> m_ComputePSOMap;

	void InitializeShader()
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
		quadInfo.FileName = "shaders/DebugQuad";
		quadInfo.bCreateVS = true;
		quadInfo.bCreatePS = true;
		quadInfo.bCreateCS = false;
		quadInfo.VSEntryPoint = "VSMain";
		quadInfo.PSEntryPoint = "PSMain";
		TShader quadShader(quadInfo);
		m_shaderMap["quadShader"] = quadShader;

		TShaderInfo DebugQuadInfo;
		DebugQuadInfo.FileName = "shaders/DebugQuad";
		DebugQuadInfo.bCreateVS = true;
		DebugQuadInfo.bCreatePS = true;
		DebugQuadInfo.bCreateCS = false;
		DebugQuadInfo.VSEntryPoint = "VSMain";
		DebugQuadInfo.PSEntryPoint = "PSMain";
		TShader DebugQuadShader(DebugQuadInfo);
		m_shaderMap["DebugQuadShader"] = DebugQuadShader;

		TShaderInfo ShadowMapDebug;
		ShadowMapDebug.FileName = "shaders/ShadowMapDebug";
		ShadowMapDebug.bCreateVS = true;
		ShadowMapDebug.bCreatePS = true;
		ShadowMapDebug.bCreateCS = false;
		ShadowMapDebug.VSEntryPoint = "VSMain";
		ShadowMapDebug.PSEntryPoint = "PSMain";
		TShader ShadowMapDebugShader(ShadowMapDebug);
		m_shaderMap["ShadowMapDebug"] = ShadowMapDebugShader;

		TShaderInfo LightGridDebug;
		LightGridDebug.FileName = "shaders/LightGridDebug";
		LightGridDebug.bCreateVS = true;
		LightGridDebug.bCreatePS = true;
		LightGridDebug.bCreateCS = false;
		LightGridDebug.VSEntryPoint = "VSMain";
		LightGridDebug.PSEntryPoint = "PSMain";
		TShader LightGridDebugShader(LightGridDebug);
		m_shaderMap["LightGridDebug"] = LightGridDebugShader;

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

		TShaderInfo GBuffersPassInfo;
		GBuffersPassInfo.FileName = "shaders/GBuffersPass";
		GBuffersPassInfo.bCreateVS = true;
		GBuffersPassInfo.bCreatePS = true;
		GBuffersPassInfo.bCreateCS = false;
		GBuffersPassInfo.VSEntryPoint = "VSMain";
		GBuffersPassInfo.PSEntryPoint = "PSMain";
		TShader GBuffersPassShader(GBuffersPassInfo);
		m_shaderMap["GBuffersPassShader"] = GBuffersPassShader;

		TShaderInfo DeferredShadingInfo;
		DeferredShadingInfo.FileName = "shaders/DeferredShading";
		DeferredShadingInfo.bCreateVS = true;
		DeferredShadingInfo.bCreatePS = true;
		DeferredShadingInfo.bCreateCS = false;
		DeferredShadingInfo.VSEntryPoint = "VSMain";
		DeferredShadingInfo.PSEntryPoint = "PSMain";
		TShader DeferredShadingShader(DeferredShadingInfo);
		m_shaderMap["DeferredShadingShader"] = DeferredShadingShader;

		TShaderInfo preDepthPassInfo;
		preDepthPassInfo.FileName = "shaders/preDepthPass";
		preDepthPassInfo.bCreateVS = true;
		preDepthPassInfo.bCreatePS = true;
		preDepthPassInfo.bCreateCS = false;
		preDepthPassInfo.VSEntryPoint = "VSMain";
		preDepthPassInfo.PSEntryPoint = "PSMain";
		TShader preDepthPassShader(preDepthPassInfo);
		m_shaderMap["preDepthPass"] = preDepthPassShader;


		TShaderInfo ForwardPulsPassInfo;
		ForwardPulsPassInfo.FileName = "shaders/ForwardPulsPass";
		ForwardPulsPassInfo.bCreateVS = true;
		ForwardPulsPassInfo.bCreatePS = true;
		ForwardPulsPassInfo.bCreateCS = false;
		ForwardPulsPassInfo.VSEntryPoint = "VSMain";
		ForwardPulsPassInfo.PSEntryPoint = "PSMain";
		TShader ForwardPulsPassShader(ForwardPulsPassInfo);
		m_shaderMap["ForwardPulsPass"] = ForwardPulsPassShader;


		TShaderInfo ShadowMapInfo;
		ShadowMapInfo.FileName = "shaders/ShadowMap";
		ShadowMapInfo.bCreateVS = true;
		ShadowMapInfo.bCreatePS = true;
		ShadowMapInfo.bCreateCS = false;
		ShadowMapInfo.VSEntryPoint = "VSMain";
		ShadowMapInfo.PSEntryPoint = "PSMain";
		TShader ShadowMapShader(ShadowMapInfo);
		m_shaderMap["ShadowMap"] = ShadowMapShader;

		InitializeComputeShader();
	}

	void InitializeComputeShader()
	{
		TShaderInfo ForwardPlusInfo;
		ForwardPlusInfo.FileName = "shaders/ForwardPulsCS";
		ForwardPlusInfo.bCreateVS = false;
		ForwardPlusInfo.bCreatePS = false;
		ForwardPlusInfo.bCreateCS = true;
		ForwardPlusInfo.CSEntryPoint = "CS_ComputeFrustums";
		TShader ForwardPulsRenderingShader(ForwardPlusInfo);
		m_shaderMap["ForwardPuls"] = ForwardPulsRenderingShader;

		ComputePSO forwardPulsPSO;
		forwardPulsPSO.SetShader(&m_shaderMap["ForwardPuls"]);
		forwardPulsPSO.Finalize();
		m_ComputePSOMap["ForwardPuls"] = std::move(forwardPulsPSO);

		TShaderInfo CullLightInfo;
		CullLightInfo.FileName = "shaders/ForwardPulsCS";
		CullLightInfo.bCreateVS = false;
		CullLightInfo.bCreatePS = false;
		CullLightInfo.bCreateCS = true;
		CullLightInfo.CSEntryPoint = "CS_main";
		TShader CullLightShader(CullLightInfo);
		m_shaderMap["CullLight"] = CullLightShader;

		ComputePSO CullLightPSO;
		CullLightPSO.SetShader(&m_shaderMap["CullLight"]);
		CullLightPSO.Finalize();
		m_ComputePSOMap["CullLight"] = std::move(CullLightPSO);
	}

	void InitializePSO()
	{
		// load shader info
		InitializeShader();

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		GraphicsPSO pso(L"model PSO");
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

		GraphicsPSO DebugQuadPso(L"DebugQuad PSO");
		DebugQuadPso.SetShader(&m_shaderMap["DebugQuadShader"]);
		DebugQuadPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		DebugQuadPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		DebugQuadPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // debug quad render first
		DebugQuadPso.SetDepthStencilState(dsvDesc);
		DebugQuadPso.SetSampleMask(UINT_MAX);
		DebugQuadPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		DebugQuadPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		DebugQuadPso.Finalize();
		m_gfxPSOMap["DebugQuadPSO"] = DebugQuadPso;

		GraphicsPSO ShadowMapDebugPso(L"ShadowMapDebug PSO");
		ShadowMapDebugPso.SetShader(&m_shaderMap["ShadowMapDebug"]);
		ShadowMapDebugPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		ShadowMapDebugPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		ShadowMapDebugPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // debug quad render first
		ShadowMapDebugPso.SetDepthStencilState(dsvDesc);
		ShadowMapDebugPso.SetSampleMask(UINT_MAX);
		ShadowMapDebugPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		ShadowMapDebugPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		ShadowMapDebugPso.Finalize();
		m_gfxPSOMap["ShadowMapDebug"] = ShadowMapDebugPso;

		GraphicsPSO ShadowMapPso(L"ShadowMap PSO");
		ShadowMapPso.SetShader(&m_shaderMap["ShadowMap"]);
		ShadowMapPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		ShadowMapPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		ShadowMapPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		ShadowMapPso.SetDepthStencilState(dsvDesc);
		ShadowMapPso.SetSampleMask(UINT_MAX);
		ShadowMapPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		ShadowMapPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		ShadowMapPso.Finalize();
		m_gfxPSOMap["ShadowMap"] = ShadowMapPso;

		GraphicsPSO LightGridDebug(L"LightGridDebug PSO");
		LightGridDebug.SetShader(&m_shaderMap["LightGridDebug"]);
		LightGridDebug.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		LightGridDebug.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		LightGridDebug.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		LightGridDebug.SetDepthStencilState(dsvDesc);
		LightGridDebug.SetSampleMask(UINT_MAX);
		LightGridDebug.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		LightGridDebug.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		LightGridDebug.Finalize();
		m_gfxPSOMap["LightGridDebug"] = LightGridDebug;


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


		GraphicsPSO preDepthPass(L"pre-depth pass PSO");
		preDepthPass.SetShader(&m_shaderMap["preDepthPass"]);
		preDepthPass.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		preDepthPass.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		preDepthPass.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		preDepthPass.SetSampleMask(UINT_MAX);
		dsvDesc.DepthEnable = TRUE;
		preDepthPass.SetDepthStencilState(dsvDesc);
		preDepthPass.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		preDepthPass.SetDepthTargetFormat(g_PreDepthPassBuffer.GetFormat());
		preDepthPass.Finalize();
		m_gfxPSOMap["preDepthPassPSO"] = preDepthPass;


		GraphicsPSO ForwardPulsPso(L"ForwardPuls PSO");
		ForwardPulsPso.SetShader(&m_shaderMap["ForwardPulsPass"]);
		ForwardPulsPso.SetInputLayout(_countof(inputElementDescs), inputElementDescs);
		ForwardPulsPso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
		ForwardPulsPso.SetBlendState(CD3DX12_BLEND_DESC(D3D12_DEFAULT));
		dsvDesc.DepthEnable = TRUE;
		dsvDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		ForwardPulsPso.SetDepthStencilState(dsvDesc);
		ForwardPulsPso.SetSampleMask(UINT_MAX);
		ForwardPulsPso.SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
		//pso.SetDepthTargetFormat(g_DepthBuffer.GetFormat());
		ForwardPulsPso.SetRenderTargetFormat(DXGI_FORMAT_R8G8B8A8_UNORM, g_DepthBuffer.GetFormat());
		ForwardPulsPso.Finalize();
		m_gfxPSOMap["ForwardPulsPass"] = ForwardPulsPso;

		}

}
