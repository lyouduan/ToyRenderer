#include "PSOManager.h"
#include "D3D12RHI.h"
#include "Shader.h"

using namespace TD3D12RHI;

namespace PSOManager
{
	std::unique_ptr<TShader> m_shader = nullptr;

	std::unordered_map<std::string, GraphicsPSO> m_gfxPSOMap;

	void InitializePSO()
	{
		{
			TShaderInfo info;
			info.FileName = "shaders/shader";

			info.bCreateVS = true;
			info.bCreatePS = true;
			info.bCreateCS = false;
			info.VSEntryPoint = "VSMain";
			info.PSEntryPoint = "PSMain";

			m_shader = std::make_unique<TShader>(info);

		}

		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		GraphicsPSO pso(L"Normal PSO");
		pso.SetShader(m_shader.get());
		pso.SetInputLayout(2, inputElementDescs);
		pso.SetRasterizerState(CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT));
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
	}

}
