#include "stdafx.h"
#include <xtl.h>
#include "OverlayFPS.h"
#include "BitmapFont.h"
#include "TextManager.h"
#include <cstdio>
#include <stdio.h>

OverlayFPS::OverlayFPS(IDirect3DDevice9* device, float screenWidth, float screenHeight)
    : m_device(device), m_screenW(screenWidth), m_screenH(screenHeight), m_enabled(false),
      m_fps(0), m_frameCount(0), m_totalFrames(0), m_displayCount(100),
      m_lastTime(0), m_lastGrowTime(0), m_font(nullptr), m_textMgr(nullptr)
{
    // Force 4096 limit for display count, ignore MAX constant if needed
    const int FORCED_MAX = 4096;
    char buf[128];
    sprintf(buf, "[OverlayFPS] FORCED_MAX=%d, STEP=%d\n", FORCED_MAX, STEP);
    OutputDebugStringA(buf);
    
    m_font = new BitmapFont(device);
    if (m_font->LoadFromFile(L"game:\\Media\\Fonts\\debug_font.fnt") && m_font->Initialize()) {
        m_textMgr = new TextManager(m_font, screenWidth, screenHeight, nullptr);
        m_enabled = true;
        m_lastTime = GetTickCount();
        m_lastGrowTime = GetTickCount();
        OutputDebugStringA("[OverlayFPS] Font loaded and initialized successfully.\n");
    } else {
        delete m_font; m_font = nullptr;
        m_textMgr = nullptr;
        m_enabled = false;
        OutputDebugStringA("[OverlayFPS] ERROR: Font failed to load or initialize.\n");
    }
}

OverlayFPS::~OverlayFPS() {
    if (m_textMgr) { delete m_textMgr; m_textMgr = nullptr; }
    if (m_font) { delete m_font; m_font = nullptr; }
}

void OverlayFPS::UpdateFrame() {
    if (!m_enabled || !m_textMgr) return;
    DWORD now = GetTickCount();
    m_frameCount++;
    m_totalFrames++;
    if (now - m_lastTime >= 1000) {
        m_fps = (int)((float)m_frameCount * 1000.0f / (float)(now - m_lastTime));
        m_lastTime = now;
        m_frameCount = 0;
    }
    if (now - m_lastGrowTime >= 3000) {
        m_lastGrowTime = now;
        int next = m_displayCount + STEP;
        // Use forced 4096 limit
        if (next > 4096) next = 4096;
        m_displayCount = next;
    }
}

void OverlayFPS::Render() {
    if (!m_enabled || !m_textMgr) return;
    m_textMgr->Begin();
    char textBuf[256];
    _snprintf(textBuf, sizeof(textBuf) - 1, "FPS: %d  Frames: %d  Sprites: %d", m_fps, m_totalFrames, m_displayCount);
    textBuf[sizeof(textBuf) - 1] = '\0';
    // TEST: Use more visible coordinates and scale
    m_textMgr->DrawTextToScreen(std::string(textBuf), 100.0f, 100.0f, 0xFFFFFFFF, 0.2f);
    m_textMgr->RenderScreen();
}
