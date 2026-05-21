#include "stdafx.h"
#include "DebugOverlay.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

DebugOverlay::DebugOverlay()
    : m_device(NULL)
    , m_visible(false)
{
}

DebugOverlay::~DebugOverlay() {
    Shutdown();
}

void DebugOverlay::Initialize(IDirect3DDevice9* device) {
    m_device = device;
    OutputDebugStringA("[DebugOverlay] Initialized\n");
}

void DebugOverlay::Shutdown() {
    m_device = NULL;
    m_gpuResults.clear();
    m_passTimings.clear();
}

void DebugOverlay::SetGPUTimerResults(const std::vector<GPUTimerResult>& results) {
    m_gpuResults = results;
}

void DebugOverlay::SetPassTimings(const PassTiming* timings, int count) {
    m_passTimings.clear();
    for (int i = 0; i < count; i++) {
        m_passTimings.push_back(timings[i]);
    }
}

void DebugOverlay::Render() {
    if (!m_visible) return;

    RenderStats();
    RenderPassTimings();
}

void DebugOverlay::RenderStats() {
    if (!m_device) return;

    char buffer[256];
    int y = 10;

    sprintf_s(buffer, "=== RENDER STATS ===");
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "Frame Time: %.2f ms", m_frameStats.frameTimeMs);
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "Draw Calls: %d", m_frameStats.drawCalls);
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "Batches: %d", m_frameStats.batchCount);
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "Triangles: %d", m_frameStats.triangles);
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "RT Switches: %d", m_frameStats.rtSwitches);
    OutputDebugStringA(buffer);

    sprintf_s(buffer, "Shader Switches: %d", m_frameStats.shaderSwitches);
    OutputDebugStringA(buffer);
}

void DebugOverlay::RenderPassTimings() {
    if (!m_device) return;

    char buffer[256];
    sprintf_s(buffer, "=== PASS TIMINGS ===");
    OutputDebugStringA(buffer);

    for (size_t i = 0; i < m_passTimings.size(); i++) {
        const PassTiming& timing = m_passTimings[i];
        sprintf_s(buffer, "%s: %.2f ms, %d draws",
                 timing.name ? timing.name : "Unknown",
                 timing.durationMs,
                 timing.drawCalls);
        OutputDebugStringA(buffer);
    }

    if (!m_gpuResults.empty()) {
        sprintf_s(buffer, "=== GPU TIMINGS ===");
        OutputDebugStringA(buffer);

        for (size_t i = 0; i < m_gpuResults.size(); i++) {
            const GPUTimerResult& result = m_gpuResults[i];
            sprintf_s(buffer, "%s: %.2f ms",
                     result.name ? result.name : "Unknown",
                     result.durationMs);
            OutputDebugStringA(buffer);
        }
    }
}

}
