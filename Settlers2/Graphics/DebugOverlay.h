#pragma once
#include <d3d9.h>
#include <string>
#include <vector>
#include "RenderFrame.h"
#include "GPUTimer.h"

namespace Graphics {

struct DebugRenderStats {
    int drawCalls;
    int batchCount;
    int triangles;
    int rtSwitches;
    int shaderSwitches;
    int textureBinds;
    int stateChanges;
    int resolves;
    int lightCount;
    float geometryPassMs;
    float lightingPassMs;
    float transparentPassMs;
    float totalMs;
    float frameTimeMs;
};

class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void SetRenderStats(const DebugRenderStats& stats);
    void SetPassStats(RenderPassType pass, const PassStats& stats);
    void SetGPUTimerResults(const std::vector<GPUTimerResult>& results);

    void Render();

    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    void Toggle() { m_visible = !m_visible; }

    void SetDPadView(int view) { m_dpadView = view; }
    int GetDPadView() const { return m_dpadView; }
    void CycleDPadView() { m_dpadView = (m_dpadView + 1) % 6; }

    void SetTextPosition(float x, float y) { m_textX = x; m_textY = y; }

private:
    void RenderText(int x, int y, const char* text);
    void RenderBar(int x, int y, int width, int height, float value, DWORD color);

    IDirect3DDevice9* m_pDevice;
    bool m_visible;
    int m_dpadView;
    float m_textX;
    float m_textY;

    DebugRenderStats m_stats;
    PassStats m_passStats[PASS_COUNT];
    std::vector<GPUTimerResult> m_gpuResults;
};

}