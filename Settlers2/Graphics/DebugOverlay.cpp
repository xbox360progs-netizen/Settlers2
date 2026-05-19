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
    ZeroMemory(m_pass_stats, sizeof(m_pass_stats));
}

DebugOverlay::~DebugOverlay() {
    Shutdown();
}

void DebugOverlay::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
}

void DebugOverlay::Shutdown() {
    m_pDevice = NULL;
}

void DebugOverlay::SetRenderStats(const DebugRenderStats& stats) {
    m_stats = stats;
}

void DebugOverlay::SetPassStats(RenderPassType pass, const PassStats& stats) {
    if (pass >= 0 && pass < PASS_COUNT) {
        m_pass_stats[pass] = stats;
    }
}

void DebugOverlay::Render() {
    if (!m_visible || !m_pDevice) return;

    char buffer[256];
    int y = (int)m_textY;

    sprintf_s(buffer, "=== RENDER STATS ===");
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Draw Calls: %d", m_stats.drawCalls);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Batches: %d", m_stats.batchCount);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Triangles: %d", m_stats.triangles);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "RT Switches: %d", m_stats.rtSwitches);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Shader Switches: %d", m_stats.shaderSwitches);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Lights: %d", m_stats.lightCount);
    RenderText(10, y, buffer); y += 25;

    sprintf_s(buffer, "=== PASS TIMES ===");
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Geometry: %.2f ms", m_stats.geometryPassMs);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Lighting: %.2f ms", m_stats.lightingPassMs);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Transparent: %.2f ms", m_stats.transparentPassMs);
    RenderText(10, y, buffer); y += 15;

    sprintf_s(buffer, "Total: %.2f ms", m_stats.totalMs);
    RenderText(10, y, buffer); y += 25;

    const char* passNames[PASS_COUNT] = {
        "Geometry", "Lighting", "Transparent", "UI", "PostFX"
    };

    sprintf_s(buffer, "=== PASS BREAKDOWN ===");
    RenderText(10, y, buffer); y += 15;

    for (int i = 0; i < PASS_COUNT; i++) {
        sprintf_s(buffer, "%s: %d draws, %d batches",
            passNames[i], m_pass_stats[i].drawCalls, m_pass_stats[i].batchCount);
        RenderText(10, y, buffer); y += 15;
    }
}

void DebugOverlay::RenderText(int x, int y, const char* text) {
}

void DebugOverlay::RenderBar(int x, int y, int width, int height, float value, DWORD color) {
}

}