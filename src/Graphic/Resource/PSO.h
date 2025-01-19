#pragma once
#include <unordered_map>
#include "d3dx12.h"
#include <assert.h>
#include "Shader.h"

class PSO
{
public:

    PSO(const wchar_t* Name) : m_Name(Name), m_RootSignature(nullptr), m_PSO(nullptr) {}

    static void DestroyAll(void);

    void SetShader(TShader* shader)
    {
        m_Shader = shader;
        m_RootSignature = m_Shader->RootSignature.Get();
    }

    void SetRootSignature(ID3D12RootSignature& BindMappings)
    {
        m_RootSignature = &BindMappings;
    }

    ID3D12RootSignature* GetRootSignature(void)
    {
        assert(m_RootSignature != nullptr);
        return m_RootSignature;
    }

    ID3D12PipelineState* GetPSO(void) const { return m_PSO; }

protected:

    const wchar_t* m_Name;

    TShader* m_Shader = nullptr;

    ID3D12RootSignature* m_RootSignature;

    ID3D12PipelineState* m_PSO;
};

class GraphicsPSO : public PSO
{
public:
    GraphicsPSO(const wchar_t* Name = L"Unnamed Graphics PSO");

    void SetBlendState(const D3D12_BLEND_DESC& BlendDesc);

    void SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc);

    void SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc);

    void SetSampleMask(UINT SampleMask);

    void SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType);

    void SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);

    void SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);

    void SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount = 1, UINT MsaaQuality = 0);

    void SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs);

    //void SetVertexShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.VS = Binary; }
    //void SetPixelShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.PS = Binary; }
    //void SetGeometryShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.GS = Binary; }
    //void SetHullShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.HS = Binary; }
    //void SetDomainShader(const D3D12_SHADER_BYTECODE& Binary) { m_PSODesc.DS = Binary; }

    void Finalize();

private:

    D3D12_GRAPHICS_PIPELINE_STATE_DESC m_PSODesc;

    std::shared_ptr<const D3D12_INPUT_ELEMENT_DESC> m_InputLayouts;
};

class ComputePSO : public PSO
{
public:
    ComputePSO(const wchar_t* Name = L"Unnamed Compute PSO");

    void Finalize();

private:

    D3D12_COMPUTE_PIPELINE_STATE_DESC m_PSODesc;
};

