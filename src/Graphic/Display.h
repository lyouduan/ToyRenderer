#pragma once
#include "D3D12RHI.h"

namespace TD3D12RHI
{
	extern Microsoft::WRL::ComPtr<IDXGISwapChain1> g_SwapCHain;
};

namespace Display
{
	// initialize the swapchain RTV
	void Initialize();

	// present render target
	void Present();

	// destroy render target 
	void Destroy();

};

