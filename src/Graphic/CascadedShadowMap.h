#pragma once
#include "D3D12PixelBuffer.h"
#include <DirectXCollision.h>
#include "D3D12Buffer.h"

class CascadedShadowMap
{
public:
	CascadedShadowMap(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format, uint32_t count);

	~CascadedShadowMap();

	void DivideFrustum(DirectX::XMFLOAT3 lightDir, float nearZ, float FarZ);

	D3D12_VIEWPORT* GetViewport() { return &m_Viewport; }
	D3D12_RECT* GetScissorRect() { return &m_SccisorRect; }

	D3D12DepthBuffer& GetShadowMap(int i) { return m_ShadowMaps[i]; }
	TD3D12Resource* GetD3D12Resource(int i) { return m_ShadowMaps[i].GetD3D12Resource(); }

	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV(int i) { return m_ShadowMaps[i].GetDSV(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(int i) { return m_ShadowMaps[i].GetSRV(); }

	// array
	std::shared_ptr<D3D12DepthBuffer>& GetShadowMapArray() { return m_ShadowMapArray; }
	TD3D12Resource* GetD3D12ResourceArray() { return m_ShadowMapArray->GetD3D12Resource(); }

	D3D12_CPU_DESCRIPTOR_HANDLE& GetShadowArrayDSV() { return m_ShadowMapArray->GetDSV(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetShadowArraySRV() { return m_ShadowMapArray->GetSRV(); }

	DirectX::XMMATRIX GetLightView(int i) const { return m_LightView[i]; }
	DirectX::XMMATRIX GetLightProj(int i) const { return m_LightProj[i]; }
	DirectX::XMMATRIX GetShadowTransform(int i) const { return m_ShadowTransform[i]; }

	std::vector<XMFLOAT2> GetFrustumVSFarZ() const { return m_FrustumVSFarZ; }

	uint32_t GetWidth(void) const { return m_Width; }
	uint32_t GetHeight(void) const { return m_Height; }
	uint32_t GetCascadeCount(void) const { return m_CascadeCount; }
	const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

	TD3D12StructuredBufferRef& GetInstanceBufferRef() { return InstanceBufferRef; }

private:

	
	void CreateShadowMaps(const std::wstring& name);

	void SetViewportAndScissorRect();

	void CreateInstanceBufferRef();

	std::vector<XMVECTOR> GetFrustumCorners(DirectX::XMMATRIX camera_view, DirectX::XMMATRIX camera_proj);

private:

	uint32_t m_Width;
	uint32_t m_Height;
	DXGI_FORMAT m_Format;
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_SccisorRect;

	uint32_t m_CascadeCount;
	std::vector<D3D12DepthBuffer> m_ShadowMaps;

	std::shared_ptr<D3D12DepthBuffer> m_ShadowMapArray;

	std::vector<XMFLOAT2> m_FrustumVSFarZ;

	// Light Space from directional light
	std::vector<DirectX::XMMATRIX> m_LightView;
	std::vector<DirectX::XMMATRIX> m_LightProj;
	std::vector<DirectX::XMMATRIX> m_ShadowTransform;

	// reduce draw call
	TD3D12StructuredBufferRef InstanceBufferRef;
};

