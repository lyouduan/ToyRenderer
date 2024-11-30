#pragma once
#include <Windows.h>
#include <cstdint>
#include "DXSample.h"
#include "GameTimer.h"

class DXSample;
class GameTimer;

class Win32Application
{
public:
	static int Run(DXSample* pSample, HINSTANCE hInstance,int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, uint32_t messagem,  WPARAM wParam, LPARAM lParam);

private:
	static HWND m_hwnd;
	
};

