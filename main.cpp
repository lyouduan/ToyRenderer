#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "Win32Application.h"
#include "GameCore.h"

#define GRS_THROW_IF_FAILED(hr) if (FAILED(hr)){ throw CGRSCOMException(hr); }

class CGRSCOMException
{
public:
	CGRSCOMException(HRESULT hr) : m_hrError(hr)
	{
	}

	HRESULT Error() const
	{
		return m_hrError;
	}

private:

	const HRESULT m_hrError;
};

#define GRS_USEPRINTF() TCHAR pBuf[1024] = {};DWORD dwRead = 0;
#define GRS_PRINTF(...) \
    StringCchPrintf(pBuf,1024,__VA_ARGS__);\
    WriteConsole(GetStdHandle(STD_OUTPUT_HANDLE),pBuf,lstrlen(pBuf),NULL,NULL);

void test()
{
	UINT nDXGIFactoryFlags = 0U;
	ComPtr<IDXGIFactory5>				pIDXGIFactory5;
	ComPtr<IDXGIAdapter1>				pIAdapter;
	ComPtr<ID3D12Device4>				pID3DDevice;

	GRS_USEPRINTF();
	try
	{
		AllocConsole(); //打开窗口程序中的命令行窗口支持

#if defined(_DEBUG)
		{//打开显示子系统的调试支持
			ComPtr<ID3D12Debug> debugController;
			if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
			{
				debugController->EnableDebugLayer();
				// 打开附加的调试支持
				nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
			}
		}
#endif
		//1、创建DXGI Factory对象
		GRS_THROW_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(&pIDXGIFactory5)));
		//2、枚举适配器，并检测其架构及存储情况
		DXGI_ADAPTER_DESC1 desc = {};
		D3D12_FEATURE_DATA_ARCHITECTURE stArchitecture = {};
		for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pIDXGIFactory5->EnumAdapters1(adapterIndex, &pIAdapter); ++adapterIndex)
		{
			pIAdapter->GetDesc1(&desc);

			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
			{//跳过软件虚拟适配器设备
				continue;
			}

			GRS_PRINTF(_T("显卡[%d]-\"%s\":独占显存[%dMB]、独占内存[%dMB]、共享内存[%dMB] ")
				, adapterIndex
				, desc.Description
				, desc.DedicatedVideoMemory / (1024 * 1024)
				, desc.DedicatedSystemMemory / (1024 * 1024)
				, desc.SharedSystemMemory / (1024 * 1024)
			);

			//创建设备，并检测架构
			//檢測顯卡架構類型
			//D3D12_FEATURE_DATA_ARCHITECTURE.UMA = true一般為集顯
			//此時若D3D12_FEATURE_DATA_ARCHITECTURE.CacheCoherentUMA = true 則表示是CC-UMA架構 GPU和CPU通過高速緩存來交換數據
			//若UMA = FALSE 一般表示為獨顯，此時CacheCoherentUMA 無意義

			GRS_THROW_IF_FAILED(D3D12CreateDevice(pIAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pID3DDevice)));
			GRS_THROW_IF_FAILED(pID3DDevice->CheckFeatureSupport(D3D12_FEATURE_ARCHITECTURE
				, &stArchitecture, sizeof(D3D12_FEATURE_DATA_ARCHITECTURE)));

			if (stArchitecture.UMA)
			{//UMA
				if (stArchitecture.CacheCoherentUMA)
				{
					GRS_PRINTF(_T("架构为(CC-UMA)"));
				}
				else
				{
					GRS_PRINTF(_T("架构为(UMA)"));
				}
			}
			else
			{//NUMA
				GRS_PRINTF(_T("架构为(NUMA)"));
			}

			if (stArchitecture.TileBasedRenderer)
			{
				GRS_PRINTF(_T(" 支持Tile-based方式渲染"));
			}

			pID3DDevice.Reset();
			GRS_PRINTF(_T("\n"));
		}
		::system("PAUSE");

		//释放命令行环境，做个干净的程序员
		FreeConsole();
	}
	catch (CGRSCOMException& e)
	{//发生了COM异常
		e;
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
	// detect GPU memory architecture
	//test();

	GameCore sample(1280, 720, L"D3D12 Engine");
	return Win32Application::Run(&sample, hInstance, nCmdShow);
}

