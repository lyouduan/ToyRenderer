#include "Display.h"
#include "stdafx.h"
#include "DXSample.h"
#include "D3D12RHI.h"
#include "Camera.h"

using namespace Microsoft::WRL;
using namespace TD3D12RHI;

namespace TD3D12RHI
{
	uint32_t g_DisplayWidth;
	uint32_t g_DisplayHeight;
	uint32_t g_frameIndex = 0;

	Camera g_Camera;
}

namespace Display
{
	void Initialize()
	{
		// obtain the DXGI factory
		ComPtr<IDXGIFactory4> factory;
		ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

		// create swap chain 
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = g_DisplayWidth;
		swapChainDesc.Height = g_DisplayHeight;
		swapChainDesc.Format = SwapChainFormat;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = FrameCount;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		ThrowIfFailed(factory->CreateSwapChainForHwnd(
			g_CommandContext.GetCommandQueue(),
			Win32Application::GetHwnd(),
			&swapChainDesc,
			&fsSwapChainDesc,
			nullptr,
			&g_SwapCHain
		));
	}

	void Resize(uint32_t width, uint32_t height)
	{
	}

	void Present()
	{
	}

}
