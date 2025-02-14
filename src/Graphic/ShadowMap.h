#pragma once
#include "D3D12PixelBuffer.h"
#include <DirectXCollision.h>

class ShadowMap
{
public:
	ShadowMap(const std::wstring& name, uint32_t width, uint32_t height, DXGI_FORMAT format);
	~ShadowMap();	

	void SetLightView(DirectX::XMFLOAT3 lightDir);

	D3D12_VIEWPORT* GetViewport() { return &m_Viewport; }
	D3D12_RECT* GetScissorRect() { return &m_SccisorRect; }

	D3D12DepthBuffer& GetShadowMap() { return m_ShadowMap; }
	TD3D12Resource* GetD3D12Resource() { return m_ShadowMap.GetD3D12Resource(); }

	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV() { return m_ShadowMap.GetDSV(); }
	D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() { return m_ShadowMap.GetSRV(); }

	DirectX::XMMATRIX GetLightView() const { return m_LightView; }
	DirectX::XMMATRIX GetLightProj() const { return m_LightProj; }
	DirectX::XMMATRIX GetShadowTransform() const { return m_ShadowTransform; }


	uint32_t GetWidth(void) const { return m_Width; }
	uint32_t GetHeight(void) const { return m_Height; }
	const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

private:
	
	void CreateShadowMap(const std::wstring& name);

	void SetViewportAndScissorRect();

private:
	
	uint32_t m_Width;
	uint32_t m_Height;
	DXGI_FORMAT m_Format;

	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_SccisorRect;

	D3D12DepthBuffer m_ShadowMap;

	// Light Space from directional light
	DirectX::XMMATRIX m_LightView;
	DirectX::XMMATRIX m_LightProj;
	DirectX::XMMATRIX m_ShadowTransform;
};

