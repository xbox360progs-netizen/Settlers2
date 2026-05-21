#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include <string>
#include "GPUTimer.h"

namespace Graphics {

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

    void SetPassTimings(const PassTiming* timings, int count);

    void Render();

private:
    void RenderStats();
    void RenderPassTimings();

    IDirect3DDevice9* m_device;
    bool m_visible;

    FrameStats m_frameStats;
    std::vector<GPUTimerResult> m_gpuResults;

    std::vector<PassTiming> m_passTimings;
};

}
