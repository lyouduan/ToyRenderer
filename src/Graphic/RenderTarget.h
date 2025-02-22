#pragma once
#include "D3D12PixelBuffer.h"
#include "Camera.h"

class TD3D12CommandContext;
class TShader;
class GraphicsPSO;
class Mesh;

class RenderTarget2D
{
public:

	RenderTarget2D(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format);

	~RenderTarget2D();

	D3D12_VIEWPORT* GetViewport() { return &m_Viewport; }

	D3D12_RECT* GetScissorRect() { return &m_ScissorRect; }

	TD3D12Resource* GetD3D12Resource() { return RenderMap.GetD3D12Resource(); }
	TD3D12Resource* GetD3D12DepthBuffer() { return DepthBuffer.GetD3D12Resource(); }

	const D3D12ColorBuffer GetColorBuffer() const { return RenderMap; }
	const D3D12DepthBuffer GetDepthBuffer() const { return DepthBuffer; }

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() const { return RenderMap.GetRTV(); }

	D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) { return RenderMap.GetSRV(); }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return RenderMap.GetSRV(); }

	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV(void) { return DepthBuffer.GetDSV(); }

	const Camera& GetCamera() const { return m_Camera; }

	void CreateDepthBuffer();

	float* GetClearColor() { return RenderMap.GetClearColor(); }

	uint32_t GetWidth(void) const { return m_Width; }
	uint32_t GetHeight(void) const { return m_Height; }
	const DXGI_FORMAT& GetFormat(void) const { return m_Format; }

private:

	void SetViewportAndScissorRect();

	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_NumMips;
	DXGI_FORMAT m_Format;

	// cube camera
	Camera m_Camera;

	// viewport
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	// cubemap
	D3D12ColorBuffer RenderMap;
	D3D12DepthBuffer DepthBuffer;
};

