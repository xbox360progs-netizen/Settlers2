#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
namespace Graphics { class RenderQueue; }
class BitmapFont;
class TextManager;

class OverlayFPS {
public:
    OverlayFPS(IDirect3DDevice9* device, float screenWidth, float screenHeight);
    ~OverlayFPS();

    void SetRenderQueue(Graphics::RenderQueue* renderQueue);
    void UpdateFrame();
    void Render();
    int GetDisplayCount() const { return m_displayCount; }

private:
    BitmapFont* m_font;
    TextManager* m_textMgr;
    Graphics::RenderQueue* m_renderQueue;
    IDirect3DDevice9* m_device;
    float m_screenW;
    float m_screenH;
    bool m_enabled;
    int m_fps;
    int m_frameCount;
    int m_totalFrames;
    int m_displayCount;
    DWORD m_lastTime;
    DWORD m_lastGrowTime;
    static const int STEP = 200;
    static const int MAX = 4096;
};
