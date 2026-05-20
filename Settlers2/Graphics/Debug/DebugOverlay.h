#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include "GPUTimer.h"
#include "RenderFrame.h"

namespace Graphics {

enum DebugViewMode {
    DEBUG_VIEW_NONE,
    DEBUG_VIEW_ALBEDO,
    DEBUG_VIEW_NORMAL,
    DEBUG_VIEW_DEPTH,
    DEBUG_VIEW_SPECULAR,
    DEBUG_VIEW_LIGHTING,
    DEBUG_VIEW_COUNT
};

struct FrameStats {
    int drawCalls;
    int batchCount;
    int triangles;
    int rtSwitches;
    int shaderSwitches;
    int textureBinds;
    int stateChanges;
    float frameTimeMs;

    FrameStats() : drawCalls(0), batchCount(0), triangles(0), rtSwitches(0),
                   shaderSwitches(0), textureBinds(0), stateChanges(0), frameTimeMs(0) {}
};

struct PassTiming {
    const char* name;
    float durationMs;
    int drawCalls;
};

class DebugOverlay {
public:
    DebugOverlay();
    ~DebugOverlay();

    void Initialize(IDirect3DDevice9* device);
    void Shutdown();

    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    void Toggle() { m_visible = !m_visible; }

    void SetFrameStats(const FrameStats& stats) { m_frameStats = stats; }
    void SetGPUTimerResults(const std::vector<GPUTimerResult>& results);

    void SetDebugViewMode(DebugViewMode mode) { m_debugView = mode; }
    DebugViewMode GetDebugViewMode() const { return m_debugView; }
    void CycleDebugView() { m_debugView = (DebugViewMode)((m_debugView + 1) % DEBUG_VIEW_COUNT); }

    void SetPassTimings(const PassTiming* timings, int count);
    void SetGBufferTextures(
        IDirect3DTexture9* pos,
        IDirect3DTexture9* normal,
        IDirect3DTexture9* albedo,
        IDirect3DTexture9* spec,
        IDirect3DTexture9* depth);

    void Render();

private:
    void RenderStats();
    void RenderPassTimings();
    void RenderMRTPreview();

    IDirect3DDevice9* m_device;
    bool m_visible;
    DebugViewMode m_debugView;

    FrameStats m_frameStats;
    std::vector<GPUTimerResult> m_gpuResults;

    std::vector<PassTiming> m_passTimings;

    IDirect3DTexture9* m_gBufferPos;
    IDirect3DTexture9* m_gBufferNormal;
    IDirect3DTexture9* m_gBufferAlbedo;
    IDirect3DTexture9* m_gBufferSpec;
    IDirect3DTexture9* m_gBufferDepth;
};

}