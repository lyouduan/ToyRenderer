#pragma once
#include "PixelBuffer.h"

class ColorBuffer : public PixelBuffer
{
public:
	ColorBuffer(DirectX::XMFLOAT4 Color = {0.0,0.0,0.0,0.0})
		:m_ClearColor(Color), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
	{
		m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;

		for(int i = 0; i < _countof(m_UAVHandle); ++i)
			m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	// create a color buffer from a swap chain buffer. Unordered access is restricted.
	void CreateFromSwapChain(ID3D12Device* device, const std::wstring& Name, ID3D12Resource* BaseResource);

	// create a color buffer
	void Create(ID3D12Device* device, const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// create a color buffer
	void CreateArray(ID3D12Device* device, const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

		// Get pre-created CPU-visible descriptor handles
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const { return m_RTVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAVHandle[0]; }

	void SetClearColor(DirectX::XMFLOAT4 ClearColor) { m_ClearColor = ClearColor; }

	void SetMsaaMode(uint32_t NumColorSamples, uint32_t NumCoverageSamples)
	{
		assert(NumCoverageSamples >= NumColorSamples);
		m_FragmentCount = NumColorSamples;
		m_SampleCount = NumCoverageSamples;
	}

	DirectX::XMFLOAT4 GetClearColor(void) const { return m_ClearColor; }

private:

	// Compute the number of texture levels needed to reduce to 1x1.  This uses
	// _BitScanReverse to find the highest set bit.  Each dimension reduces by
	// half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
	// as the dimension 511 (0x1FF).
	static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
	{
		uint32_t HighBit;
		_BitScanReverse((unsigned long*)&HighBit, Width | Height);
		return HighBit + 1;
	}

	void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);


	// clear color
	DirectX::XMFLOAT4 m_ClearColor;

	// CPU handle 
	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
	
	// buffer info
	uint32_t m_NumMipMaps; // number of texture sublevels
	uint32_t m_FragmentCount;
	uint32_t m_SampleCount;
};

