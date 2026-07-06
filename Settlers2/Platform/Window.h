#pragma once

// NOTE: Names avoid Win32 macro conflicts (CreateWindow, ShowWindow, DestroyWindow are #defines in windows.h).

namespace Platform {

    // Platform-independent window abstraction.
    //
    // On Win32:  creates an actual HWND window.
    // On Xbox:   stub — no window exists; all rendering goes direct to framebuffer.

    // Window handle type (HWND on Win32, void* on Xbox).
    typedef void* WindowHandle;

    // Create a window with the given title and dimensions.
    // Returns NULL on failure.
    WindowHandle OpenWindow(const char* title, int width, int height);

    // Show the window (make it visible on Win32; no-op on Xbox).
    void ShowPlatformWindow(WindowHandle hWnd);

    // Process pending window messages (Win32: PeekMessage/DispatchMessage loop).
    // Returns false if WM_QUIT received.
    bool ProcessWindowMessages(WindowHandle hWnd);

    // Destroy the window.
    void CloseWindow(WindowHandle hWnd);

    // Get the window's client area dimensions.
    void GetPlatformWindowSize(WindowHandle hWnd, int* width, int* height);

} // namespace Platform