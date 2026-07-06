#include <windows.h>
#include "../Window.h"

namespace Platform {

    static const char* kWindowClass = "Settlers2Window";
    static bool s_classRegistered = false;

    // Forward declare window proc.
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    static void RegisterWindowClass()
    {
        if (s_classRegistered) return;

        WNDCLASSEX wc;
        wc.cbSize        = sizeof(WNDCLASSEX);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = WndProc;
        wc.cbClsExtra    = 0;
        wc.cbWndExtra    = 0;
        wc.hInstance     = GetModuleHandle(NULL);
        wc.hIcon         = NULL;
        wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        wc.lpszMenuName  = NULL;
        wc.lpszClassName = kWindowClass;
        wc.hIconSm       = NULL;

        RegisterClassEx(&wc);
        s_classRegistered = true;
    }

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg) {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;

            case WM_CLOSE:
                DestroyWindow(hWnd);
                return 0;

            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }
    }

    WindowHandle OpenWindow(const char* title, int width, int height)
    {
        RegisterWindowClass();

        RECT rect = { 0, 0, width, height };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        // Use CreateWindowExA explicitly to avoid macro expansion conflict.
        HWND hWnd = CreateWindowExA(
            0,
            kWindowClass,
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            NULL, NULL,
            GetModuleHandle(NULL),
            NULL
        );

        return (WindowHandle)hWnd;
    }

    void ShowPlatformWindow(WindowHandle hWnd)
    {
        ShowWindow((HWND)hWnd, SW_SHOW);
    }

    bool ProcessWindowMessages(WindowHandle hWnd)
    {
        (void)hWnd;
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) return false;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return true;
    }

    void CloseWindow(WindowHandle hWnd)
    {
        DestroyWindow((HWND)hWnd);
    }

    void GetPlatformWindowSize(WindowHandle hWnd, int* width, int* height)
    {
        RECT rect;
        GetClientRect((HWND)hWnd, &rect);
        *width  = rect.right - rect.left;
        *height = rect.bottom - rect.top;
    }

} // namespace Platform