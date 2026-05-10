#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <string>
class BitmapFont;
class TextManager;

class OverlayFPS {
public:
    OverlayFPS(IDirect3DDevice9* device, float screenWidth, float screenHeight);
    ~OverlayFPS();

    void UpdateFrame();
    void Render();
    int GetDisplayCount() const { return m_displayCount; }

private:
    BitmapFont* m_font;
    TextManager* m_textMgr;
    IDirect3DDevice9* m_device;
    float m_screenW;
    float m_screenH;
    bool m_enabled;
    int m_fps;
    int m_frameCount;   // frames in current second
    int m_totalFrames;    // total frames since start
    int m_displayCount; // current number of sprites displayed (for overlay)
    DWORD m_lastTime;
    DWORD m_lastGrowTime;
    static const int STEP = 200;
    static const int MAX = 4096;
};
