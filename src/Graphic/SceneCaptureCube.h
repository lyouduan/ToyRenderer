#pragma once
#include "D3D12PixelBuffer.h"
#include "Camera.h"

class TD3D12CommandContext;
class TShader;
class GraphicsPSO;
class Mesh;

class SceneCaptureCube
{
public:

	SceneCaptureCube(const std::wstring& name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format);

	~SceneCaptureCube();

	D3D12_VIEWPORT* GetViewport() { return &m_Viewport; }

	D3D12_RECT* GetScissorRect() { return &m_ScissorRect; }

	TD3D12Resource* GetD3D12Resource() { return CubeMap.GetD3D12Resource(); }
	TD3D12Resource* GetD3D12DepthBuffer() { return DepthBuffer.GetD3D12Resource(); }
	
	const D3D12CubeBuffer GetCubeBuffer() const { return CubeMap; }
	const D3D12DepthBuffer GetDepthBuffer() const { return DepthBuffer; }

	const D3D12_CPU_DESCRIPTOR_HANDLE* GetRTV() const { return CubeMap.GetRTV(); }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(int i) const { return CubeMap.GetRTV(i);}

	D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) { return CubeMap.GetSRV(); }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return CubeMap.GetSRV(); }

	D3D12_CPU_DESCRIPTOR_HANDLE& GetDSV(void)  { return DepthBuffer.GetDSV(); }

	const Camera& GetCamera(UINT i) const { return m_Camera[i]; }
	
	void CreateCubeCamera(XMFLOAT3 pos, float nearZ, float farZ);
	void CreateDepthBuffer();

	bool& GetIsUseCubeMap() { return bUseCubeMap; }
	void SetIsUseCubeMap(bool IsUse) { bUseCubeMap = IsUse; }

private:

	void SetViewportAndScissorRect();

	uint32_t m_Width;
	uint32_t m_Height;
	uint32_t m_NumMips;
	DXGI_FORMAT m_Format;

	// cube camera
	Camera m_Camera[6];

	// viewport
	D3D12_VIEWPORT m_Viewport;
	D3D12_RECT m_ScissorRect;

	// cubemap
	D3D12CubeBuffer CubeMap;
	D3D12DepthBuffer DepthBuffer;

	bool bUseCubeMap = false;
};

