#pragma once
#include "DXSamplerHelper.h"
#include "Win32Application.h"
#include "stdafx.h"
#include "GameTimer.h"

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"

class DXSample
{
public:
	DXSample(uint32_t width, uint32_t height, std::wstring name);
	virtual ~DXSample();

	virtual void OnInit() = 0;
	virtual void OnUpdate(const GameTimer& gt) = 0;
	virtual void OnRender() = 0;
	virtual void OnDestroy() = 0;

	virtual void OnKeyDown(UINT8 /*key*/) {}
	virtual void OnKeyUp(UINT8 /*key*/) {}

	// Accessors
	uint32_t GetWidth() const { return m_width; }
	uint32_t GetHeight() const { return m_height; }
	const wchar_t* GetTitle() const { return m_title.c_str(); }
	GameTimer GetTimer() const { return m_Timer; }

	void ParseCommandLineArgs(_In_reads_(argc) wchar_t* argv[], int argc);
	
	void CalculateFrameStats();

protected:
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter(
		_In_ IDXGIFactory1* pFactory,
		_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	void SetCustomWindowText(LPCWSTR text);

	// Viewport dimensions
	uint32_t m_width;
	uint32_t m_height;
	float m_aspectRatio;

	// Adapter info
	bool m_useWarpDevice;

	// Game Timer
	GameTimer m_Timer;

private:

	// Root assets path.
	std::wstring m_assetsPath;

	// Window title.
	std::wstring m_title;
};

