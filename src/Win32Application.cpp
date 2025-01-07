#include "Win32Application.h"
#include "GameTimer.h"
#include "D3D12RHI.h"
#include "Display.h"
#include "ImGuiManager.h"

HWND Win32Application::m_hwnd = nullptr;
Keyboard Win32Application::kbd;

int Win32Application::Run(DXSample* pSample, HINSTANCE hInstance, int nCmdShow)
{
    // Parse the command line parameters
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    pSample->ParseCommandLineArgs(argv, argc);
    LocalFree(argv);

    // Initialize the window class
    WNDCLASSEX windowClass = { 0 };
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = hInstance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = L"DXSampleClass";
    RegisterClassEx(&windowClass);

    RECT windowRect = { 0, 0, static_cast<LONG>(pSample->GetWidth()), static_cast<LONG>(pSample->GetHeight()) };
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hwnd = CreateWindow(
        windowClass.lpszClassName,
        pSample->GetTitle(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        hInstance,
        pSample
    );

    // initialze the sample
    pSample->OnInit();
    
    // initialze ImGui
    ImGuiManager::InitImGui();

    ShowWindow(m_hwnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        // If there are Window messages then process them.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        // Otherwise, do animation/game stuff.
        else
        {
            pSample->GetTimer().Tick();

        }
        if (kbd.KeyIsPressed(VK_SPACE))
        {
            MessageBox(nullptr, L"Something Happon!", L"The alt key was pressed", MB_OK | MB_ICONEXCLAMATION);
        }
    }

    pSample->OnDestroy();

    return static_cast<char>(msg.wParam);
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT Win32Application::WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    DXSample* pSample = reinterpret_cast<DXSample*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    
    // ImGui
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }
   
    switch (message)
    {
    case WM_CREATE:
        {
            // Save the DXSample* passed in to CreateWindow.
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        return 0;
    
    case WM_PAINT:
        if (pSample)
        {
            pSample->OnUpdate(pSample->GetTimer());
            pSample->OnRender();
        }
        return 0;

    case WM_SIZE:
        {
           // onResize()
        }
        return 0;

    case WM_KILLFOCUS:
        kbd.ClearState();
        break;
    case WM_KEYDOWN:
        if (ImGui::GetIO().WantCaptureKeyboard)
        {
            break;
        }
        if (!(lParam & 0x40000000) || kbd.AutoRepeatIsEnabled()) // filter autorepeat
        {
            kbd.OnKeyPressed(static_cast<unsigned char>(wParam));
        }
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        // stifle this keyboard message if imgui wants to capture
        if (ImGui::GetIO().WantCaptureKeyboard)
        {
            break;
        }
        kbd.OnKeyReleased(static_cast<unsigned char>(wParam));
        break;
    case WM_CHAR:
        // stifle this keyboard message if imgui wants to capture
        if (ImGui::GetIO().WantCaptureKeyboard)
        {
            break;
        }
        kbd.OnChar(static_cast<unsigned char>(wParam));
        break;
    /*********** END KEYBOARD MESSAGES ***********/

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}
