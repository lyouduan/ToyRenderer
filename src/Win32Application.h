#pragma once
#include <Windows.h>
#include <cstdint>
#include "DXSample.h"
#include "GameTimer.h"
#include "Keyboard.h"

class DXSample;
class GameTimer;

class Win32Application
{
public:
	static int Run(DXSample* pSample, HINSTANCE hInstance,int nCmdShow);
	static HWND GetHwnd() { return m_hwnd; }

	static Keyboard& GetKeyboard() { return kbd; }

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, uint32_t messagem,  WPARAM wParam, LPARAM lParam);

	static Keyboard kbd;

private:
	static HWND m_hwnd;
	
};

