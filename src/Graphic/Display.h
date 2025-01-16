#pragma once
#include "D3D12RHI.h"
#define SwapChainFormat DXGI_FORMAT_R8G8B8A8_UNORM

class Camera;

namespace TD3D12RHI
{
	extern Microsoft::WRL::ComPtr<IDXGISwapChain1> g_SwapCHain;
	extern uint32_t g_DisplayWidth;
	extern uint32_t g_DisplayHeight;
	extern uint32_t g_frameIndex;

	extern Camera g_Camera;
};

namespace Display
{
	// initialize the swapchain RTV
	void Initialize();

	void Resize(uint32_t width, uint32_t height);

	// present render target
	void Present();

	// destroy render target 
	void Destroy();

};

