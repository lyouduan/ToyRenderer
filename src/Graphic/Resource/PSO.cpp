#include "PSO.h"
#include "hash.h"
#include <map>
#include <mutex>
#include "DXSamplerHelper.h"

using Microsoft::WRL::ComPtr;

static std::map< size_t, ComPtr<ID3D12PipelineState> > s_GraphicsPSOHashMap;
static std::map< size_t, ComPtr<ID3D12PipelineState> > s_ComputePSOHashMap;

GraphicsPSO::GraphicsPSO(const wchar_t* Name)
	: PSO(Name)
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
    m_PSODesc.SampleMask = 0xFFFFFFFFu;
    m_PSODesc.SampleDesc.Count = 1;
    m_PSODesc.InputLayout.NumElements = 0;
}

void GraphicsPSO::SetBlendState(const D3D12_BLEND_DESC& BlendDesc)
{
	m_PSODesc.BlendState = BlendDesc;
}

void GraphicsPSO::SetRasterizerState(const D3D12_RASTERIZER_DESC& RasterizerDesc)
{
	m_PSODesc.RasterizerState = RasterizerDesc;
}

void GraphicsPSO::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& DepthStencilDesc)
{
	m_PSODesc.DepthStencilState = DepthStencilDesc;
}

void GraphicsPSO::SetSampleMask(UINT SampleMask)
{
	m_PSODesc.SampleMask = SampleMask;
}

void GraphicsPSO::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType)
{
	assert(TopologyType != D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED && "Can't draw with undefined topology");
	m_PSODesc.PrimitiveTopologyType = TopologyType;
}

void GraphicsPSO::SetDepthTargetFormat(DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(0, nullptr, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormat(DXGI_FORMAT RTVFormat, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    SetRenderTargetFormats(1, &RTVFormat, DSVFormat, MsaaCount, MsaaQuality);
}

void GraphicsPSO::SetRenderTargetFormats(UINT NumRTVs, const DXGI_FORMAT* RTVFormats, DXGI_FORMAT DSVFormat, UINT MsaaCount, UINT MsaaQuality)
{
    assert(NumRTVs == 0 || RTVFormats != nullptr && "Null format array conflicts with non-zero length");
    for (UINT i = 0; i < NumRTVs; ++i)
    {
        assert(RTVFormats[i] != DXGI_FORMAT_UNKNOWN);
        m_PSODesc.RTVFormats[i] = RTVFormats[i];
    }
    for (UINT i = NumRTVs; i < m_PSODesc.NumRenderTargets; ++i)
        m_PSODesc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
    m_PSODesc.NumRenderTargets = NumRTVs;
    m_PSODesc.DSVFormat = DSVFormat;
    m_PSODesc.SampleDesc.Count = MsaaCount;
    m_PSODesc.SampleDesc.Quality = MsaaQuality;
}

void GraphicsPSO::SetInputLayout(UINT NumElements, const D3D12_INPUT_ELEMENT_DESC* pInputElementDescs)
{
    m_PSODesc.InputLayout.NumElements = NumElements;

    if (NumElements > 0)
    {
        D3D12_INPUT_ELEMENT_DESC* NewElements = (D3D12_INPUT_ELEMENT_DESC*)malloc(sizeof(D3D12_INPUT_ELEMENT_DESC) * NumElements);
        memcpy(NewElements, pInputElementDescs, NumElements * sizeof(D3D12_INPUT_ELEMENT_DESC));
        m_InputLayouts.reset((const D3D12_INPUT_ELEMENT_DESC*)NewElements);
    }
    else
        m_InputLayouts = nullptr;
}

void GraphicsPSO::Finalize()
{
    m_PSODesc.VS = D3D12_SHADER_BYTECODE{ m_Shader->ShaderPass.at("VS")->GetBufferPointer(), m_Shader->ShaderPass.at("VS")->GetBufferSize() };
    m_PSODesc.PS = D3D12_SHADER_BYTECODE{ m_Shader->ShaderPass.at("PS")->GetBufferPointer(), m_Shader->ShaderPass.at("PS")->GetBufferSize() };

    m_PSODesc.pRootSignature = m_RootSignature;

    // Make sure the root signature is finalized first
    assert(m_RootSignature != nullptr);
    assert(m_PSODesc.pRootSignature != nullptr);

    m_PSODesc.InputLayout.pInputElementDescs = nullptr;
    size_t HashCode = Utility::HashState(&m_PSODesc);
    HashCode = Utility::HashState(m_InputLayouts.get(), m_PSODesc.InputLayout.NumElements, HashCode);
    m_PSODesc.InputLayout.pInputElementDescs = m_InputLayouts.get();

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);
        auto iter = s_GraphicsPSOHashMap.find(HashCode);

        // Reserve space so the next inquiry will find that someone got here first.
        if (iter == s_GraphicsPSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_GraphicsPSOHashMap[HashCode].GetAddressOf();
        }
        else
            PSORef = iter->second.GetAddressOf();
    }

    if (firstCompile)
    {
        assert(m_PSODesc.DepthStencilState.DepthEnable != (m_PSODesc.DSVFormat == DXGI_FORMAT_UNKNOWN));
        ThrowIfFailed(TD3D12RHI::g_Device->CreateGraphicsPipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_GraphicsPSOHashMap[HashCode].Attach(m_PSO);
        m_PSO->SetName(m_Name);
    }
    else
    {
        while (*PSORef == nullptr)
            std::this_thread::yield();
        m_PSO = *PSORef;
    }
}

ComputePSO::ComputePSO(const wchar_t* Name)
    : PSO(Name)
{
    ZeroMemory(&m_PSODesc, sizeof(m_PSODesc));
    m_PSODesc.NodeMask = 1;
}

void ComputePSO::Finalize()
{
    // make sure the root signature is finalized first
    m_PSODesc.CS = D3D12_SHADER_BYTECODE{ m_Shader->ShaderPass.at("CS")->GetBufferPointer(), m_Shader->ShaderPass.at("CS")->GetBufferSize() };

    m_PSODesc.pRootSignature = m_Shader->RootSignature.Get();

    assert(m_PSODesc.pRootSignature != nullptr);
    assert(m_RootSignature != nullptr);

    size_t HashCode = Utility::HashState(&m_PSODesc);

    ID3D12PipelineState** PSORef = nullptr;
    bool firstCompile = false;
    {
        static std::mutex s_HashMapMutex;
        std::lock_guard<std::mutex> CS(s_HashMapMutex);

        auto iter = s_ComputePSOHashMap.find(HashCode);
        // Reserve space so  the next inquiry will find that someone got here first
        if (iter == s_ComputePSOHashMap.end())
        {
            firstCompile = true;
            PSORef = s_ComputePSOHashMap[HashCode].GetAddressOf();
        }
        else
        {
            PSORef = iter->second.GetAddressOf();
        }
    }

    if (firstCompile)
    {
        ThrowIfFailed(TD3D12RHI::g_Device->CreateComputePipelineState(&m_PSODesc, IID_PPV_ARGS(&m_PSO)));
        s_ComputePSOHashMap[HashCode].Attach(m_PSO);
        m_PSO->SetName(m_Name);
    }
    else
    {
        while (*PSORef == nullptr)
            std::this_thread::yield();
        m_PSO = *PSORef;
    }
}

