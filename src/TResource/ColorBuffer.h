#pragma once
#include "PixelBuffer.h"
#include <DirectXMath.h>

class ColorBuffer : public PixelBuffer
{
public:
	ColorBuffer(DirectX::XMFLOAT4 clearColor = DirectX::XMFLOAT4(0, 0, 0, 0))
		: m_ClearColor(clearColor), m_NumMipMaps(0), m_FrangmentCount(1), m_SampleCount(1)
	{
		m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
		for(int i = 0; i <_countof(m_UAVHandle);++i)
			m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
	}

	// create a color buffer from a swap chain buffer
	void CreateFromSwapChain(const std::wstring& Name, ID3D12Resource* BaseResource);

	// create a color buffer
	void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	// create a color buffer
	void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount, DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);

	const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV() const { return m_SRVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV() const { return m_RTVHandle; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV() const { return m_UAVHandle[0]; }
	const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(int i) const 
	{ 
		assert(i < _countof(m_UAVHandle) && i >= 0); 
		return m_UAVHandle[i]; 
	}

	void SetClearColor(DirectX::XMFLOAT4 ClearColor) { m_ClearColor = ClearColor; }

	void SetMsaaMode(uint32_t NumColorsSamples, uint32_t NumCoverageSamples)
	{
		assert(NumCoverageSamples >= NumColorsSamples);
		m_FrangmentCount = NumColorsSamples;
		m_SampleCount = NumCoverageSamples;
	}

	DirectX::XMFLOAT4 GetClearColor() const { return m_ClearColor; }

	// generate mipmaps
	// void GenerateMipMaps();

protected:

	D3D12_RESOURCE_FLAGS CombineResourceFlags() const
	{
		D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

		if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FrangmentCount == 1)
			Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

		return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
	}

	static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
	{
		uint32_t HighBit;
		_BitScanReverse((unsigned long*)&HighBit, Width | Height);
		return HighBit + 1;
	}

	void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

	DirectX::XMFLOAT4 m_ClearColor;

	D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
	uint32_t m_NumMipMaps;
	uint32_t m_FrangmentCount;
	uint32_t m_SampleCount;
};

