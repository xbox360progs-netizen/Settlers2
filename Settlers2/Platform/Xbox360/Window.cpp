#include "../Window.h"

namespace Platform {

    WindowHandle OpenWindow(const char* title, int width, int height)
    {
        (void)title;
        (void)width;
        (void)height;
        // Xbox 360 has no OS window — rendering goes direct to framebuffer.
        return (WindowHandle)1;  // non-nULL placeholder
    }

    void ShowPlatformWindow(WindowHandle hWnd)
    {
        (void)hWnd;
    }

    bool ProcessWindowMessages(WindowHandle hWnd)
    {
        (void)hWnd;
        return true;
    }

    void CloseWindow(WindowHandle hWnd)
    {
        (void)hWnd;
    }

    void GetPlatformWindowSize(WindowHandle hWnd, int* width, int* height)
    {
        (void)hWnd;
        // Xbox 360 has fixed 1280x720 framebuffer.
        *width  = 1280;
        *height = 720;
    }

} // namespace Platform