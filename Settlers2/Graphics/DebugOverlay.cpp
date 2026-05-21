#include "stdafx.h"
#include "DebugOverlay.h"

namespace Graphics {

DebugOverlay::DebugOverlay()
    : m_pDevice(NULL)
    , m_visible(false)
    , m_dpadView(0)
    , m_textX(10.0f)
    , m_textY(10.0f)
{
    ZeroMemory(&m_stats, sizeof(m_stats));
    ZeroMemory(m_passStats, sizeof(m_passStats));
}

DebugOverlay::~DebugOverlay() {
    Shutdown();
}

void DebugOverlay::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
}

void DebugOverlay::Shutdown() {
    m_pDevice = NULL;
    m_gpuResults.clear();
}

void DebugOverlay::SetRenderStats(const DebugRenderStats& stats) {
    m_stats = stats;
}

void DebugOverlay::SetPassStats(RenderPassType pass, const PassStats& stats) {
    if (pass >= 0 && pass < PASS_COUNT) {
        m_passStats[pass] = stats;
    }
}

void DebugOverlay::SetGPUTimerResults(const std::vector<GPUTimerResult>& results) {
    m_gpuResults = results;
}

void DebugOverlay::Render() {
    if (!m_visible || !m_pDevice) return;

    char buffer[256];
    int y = (int)m_textY;

    sprintf_s(buffer, "=== RENDER STATS ===");
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Frame Time: %.2f ms", m_stats.frameTimeMs);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Draw Calls: %d", m_stats.drawCalls);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Batches: %d", m_stats.batchCount);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Triangles: %d", m_stats.triangles);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "State Changes: %d", m_stats.stateChanges);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "RT Switches: %d", m_stats.rtSwitches);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Shader Switches: %d", m_stats.shaderSwitches);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Texture Binds: %d", m_stats.textureBinds);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Resolves: %d", m_stats.resolves);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Lights: %d", m_stats.lightCount);
    RenderText(10, y, buffer); y += 25;

    if (!m_gpuResults.empty()) {
        sprintf_s(buffer, "=== GPU TIMING ===");
        RenderText(10, y, buffer); y += 15;

        for (size_t i = 0; i < m_gpuResults.size(); i++) {
            const GPUTimerResult& result = m_gpuResults[i];
            sprintf_s(buffer, "%s: %.2f ms", result.name ? result.name : "Unknown", result.durationMs);
            RenderText(10, y, buffer); y += 15;
        }
        y += 10;
    }

    sprintf_s(buffer, "=== PASS BREAKDOWN ===");
    RenderText(10, y, buffer); y += 15;

    const char* passNames[PASS_COUNT] = {
        "Geometry", "Lighting", "Transparent", "UI"
    };

    for (int i = 0; i < PASS_COUNT; i++) {
        sprintf_s(buffer, "%s: %d draws, %.2f ms",
            passNames[i], m_passStats[i].drawCalls, m_passStats[i].gpuTimeMs);
        RenderText(10, y, buffer); y += 15;
    }
}

void DebugOverlay::RenderText(int x, int y, const char* text) {
}

void DebugOverlay::RenderBar(int x, int y, int width, int height, float value, DWORD color) {
}

}