#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "./src/Win32Application.h"
#include "./src/D3D12HelloTriangle.h"
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	D3D12HelloTriangle sample(1280, 720, L"D3D12 Hello Triangle");
	return Win32Application::Run(&sample, hInstance, nCmdShow);
}