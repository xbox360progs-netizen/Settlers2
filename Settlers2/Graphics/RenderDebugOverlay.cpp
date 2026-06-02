#include "stdafx.h"
#include "RenderDebugOverlay.h"
#include <stdio.h>
#include <time.h>

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderDebugOverlay::RenderDebugOverlay()
    : m_pDevice(NULL), m_overlayEnabled(false),
      m_showFPS(true), m_showDrawCalls(true), m_showTriangles(false),
      m_showLightCount(false), m_showMemory(false), m_showGBuffer(false),
      m_fps(0.0f), m_frameTimeAccum(0.0f), m_frameCount(0),
      m_lastGBufferIndex(-1), m_gBufferCaptured(false), m_gBufferSnapshot(NULL) {
    m_stats.Reset();
    m_lastFrameStats.Reset();
}

RenderDebugOverlay::~RenderDebugOverlay() {
    Shutdown();
}

void RenderDebugOverlay::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;

    QueryPerformanceFrequency(&m_frequency);

    char buf[256];
    sprintf(buf, "[RenderDebugOverlay] Initialized (freq=%lld)\n", m_frequency.QuadPart);
    OutputDebugStringA(buf);
}

void RenderDebugOverlay::Shutdown() {
    m_overlayLines.clear();
    m_gBufferSnapshot = NULL;

    OutputDebugStringA("[RenderDebugOverlay] Shutdown complete\n");
}

void RenderDebugOverlay::BeginFrame() {
    m_stats.Reset();
    QueryPerformanceCounter(&m_frameStart);

    m_overlayLines.clear();
}

void RenderDebugOverlay::EndFrame() {
    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);

    float frameTimeMs = (float)(endTime.QuadPart - m_frameStart.QuadPart) * 1000.0f / (float)m_frequency.QuadPart;
    m_stats.frameTime = frameTimeMs;

    m_frameTimeAccum += frameTimeMs;
    m_frameCount++;

    if (m_frameTimeAccum >= 1000.0f) {
        m_fps = m_frameCount * 1000.0f / m_frameTimeAccum;
        m_frameTimeAccum = 0.0f;
        m_frameCount = 0;
    }

    m_lastFrameStats = m_stats;

    if (m_overlayEnabled) {
        FormatStats();
    }
}

void RenderDebugOverlay::UpdateFPS(float deltaTime) {
}

void RenderDebugOverlay::RecordDrawCall(int vertexCount, int primitiveCount) {
    m_stats.drawCalls++;
    m_stats.vertices += vertexCount;
    m_stats.triangles += primitiveCount;
}

void RenderDebugOverlay::RecordTextureSwitch() {
    m_stats.textureSwitches++;
}

void RenderDebugOverlay::RecordShaderSwitch() {
    m_stats.shaderSwitches++;
}

void RenderDebugOverlay::RecordStateChange() {
    m_stats.stateChanges++;
}

void RenderDebugOverlay::RecordRenderTargetSwitch() {
    m_stats.rtSwitches++;
}

void RenderDebugOverlay::RecordLightCount(int count) {
    m_stats.lightCount = count;
}

void RenderDebugOverlay::RecordShadowMapCount(int count) {
    m_stats.shadowMapCount = count;
}

void RenderDebugOverlay::RecordBatch(int vertexCount, bool instanced) {
    m_stats.batchCount++;
    if (instanced) {
        m_stats.instancedDraws++;
    }
}

void RenderDebugOverlay::FormatStats() {
    char buf[512];

    if (m_showFPS) {
        sprintf(buf, "FPS: %.1f", m_fps);
        m_overlayLines.push_back(buf);
    }

    if (m_showDrawCalls) {
        sprintf(buf, "Draw Calls: %d", m_stats.drawCalls);
        m_overlayLines.push_back(buf);

        sprintf(buf, "Batches: %d", m_stats.batchCount);
        m_overlayLines.push_back(buf);

        sprintf(buf, "Sprites: %d", m_stats.spriteCount);
        m_overlayLines.push_back(buf);

        sprintf(buf, "VB Flushes: %d", m_stats.vbFlushes);
        m_overlayLines.push_back(buf);

        sprintf(buf, "Texture Switches: %d", m_stats.textureSwitches);
        m_overlayLines.push_back(buf);

        sprintf(buf, "Shader Switches: %d", m_stats.shaderSwitches);
        m_overlayLines.push_back(buf);

        sprintf(buf, "State Changes: %d", m_stats.stateChanges);
        m_overlayLines.push_back(buf);
    }

    if (m_showTriangles) {
        sprintf(buf, "Triangles: %d", m_stats.triangles);
        m_overlayLines.push_back(buf);
    }

    if (m_showLightCount) {
        sprintf(buf, "Lights: %d (Shadows: %d)", m_stats.lightCount, m_stats.shadowMapCount);
        m_overlayLines.push_back(buf);
    }

    sprintf(buf, "Frame Time: %.2f ms", m_stats.frameTime);
    m_overlayLines.push_back(buf);

    if (m_stats.gpuTime > 0) {
        sprintf(buf, "GPU Time: %.2f ms", m_stats.gpuTime);
        m_overlayLines.push_back(buf);
    }

    if (m_stats.cpuTime > 0) {
        sprintf(buf, "CPU Time: %.2f ms", m_stats.cpuTime);
        m_overlayLines.push_back(buf);
    }

    if (m_stats.instancedDraws > 0) {
        sprintf(buf, "Instanced: %d", m_stats.instancedDraws);
        m_overlayLines.push_back(buf);
    }
}

void RenderDebugOverlay::RenderOverlay(int screenWidth, int screenHeight) {
    if (!m_overlayEnabled || m_overlayLines.empty()) return;

    int x = 10;
    int y = 10;
    int lineHeight = 16;

    OutputDebugStringA("[RenderDebugOverlay] === Overlay ===\n");
    for (size_t i = 0; i < m_overlayLines.size(); i++) {
        OutputDebugStringA((m_overlayLines[i] + "\n").c_str());
    }
}

void RenderDebugOverlay::DrawText(int x, int y, const char* text, DWORD color) {
    OutputDebugStringA(text);
}

void RenderDebugOverlay::DrawOverlayBackground(int x, int y, int width, int height) {
}

void RenderDebugOverlay::CaptureGBufferSnapshot(int gbufferIndex) {
    m_lastGBufferIndex = gbufferIndex;
    m_gBufferCaptured = true;
}

void RenderDebugOverlay::RenderGBufferView(int gbufferIndex, int screenWidth, int screenHeight) {
    if (!m_gBufferCaptured) return;

    char buf[256];
    sprintf(buf, "[RenderDebugOverlay] Rendering GBuffer view %d\n", gbufferIndex);
    OutputDebugStringA(buf);
}

// === GPUProfiler ===

GPUProfiler::GPUProfiler()
    : m_pDevice(NULL), m_enabled(false) {
    QueryPerformanceFrequency(&m_frequency);
}

GPUProfiler::~GPUProfiler() {
    Shutdown();
}

void GPUProfiler::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    m_pDevice = pDevice;
    m_sections.clear();
    m_activeTimers.clear();

    OutputDebugStringA("[GPUProfiler] Initialized\n");
}

void GPUProfiler::Shutdown() {
    m_sections.clear();
    m_activeTimers.clear();

    OutputDebugStringA("[GPUProfiler] Shutdown complete\n");
}

void GPUProfiler::BeginSection(const char* name) {
    if (!m_enabled) return;

    SectionTimer timer;
    timer.name = name;
    QueryPerformanceCounter(&timer.startTime);

    m_activeTimers.push_back(timer);
}

void GPUProfiler::EndSection() {
    if (!m_enabled || m_activeTimers.empty()) return;

    SectionTimer timer = m_activeTimers.back();
    m_activeTimers.pop_back();

    LARGE_INTEGER endTime;
    QueryPerformanceCounter(&endTime);

    float durationMs = (float)(endTime.QuadPart - timer.startTime.QuadPart) * 1000.0f / (float)m_frequency.QuadPart;

    RecordSection(timer.name.c_str(), durationMs);
}

void GPUProfiler::BeginFrame() {
    m_sections.clear();
    QueryPerformanceCounter(&m_frameStart);
}

void GPUProfiler::EndFrame() {
    m_lastFrameSections = m_sections;
}

float GPUProfiler::GetSectionTime(const char* name) {
    for (size_t i = 0; i < m_lastFrameSections.size(); i++) {
        if (m_lastFrameSections[i].name == std::string(name)) {
            return m_lastFrameSections[i].time;
        }
    }
    return 0.0f;
}

float GPUProfiler::GetTotalTime() {
    float total = 0.0f;
    for (size_t i = 0; i < m_lastFrameSections.size(); i++) {
        total += m_lastFrameSections[i].time;
    }
    return total;
}

void GPUProfiler::StartTimer(const char* name) {
}

void GPUProfiler::StopTimer(const char* name) {
}

void GPUProfiler::RecordSection(const char* name, float durationMs) {
    for (size_t i = 0; i < m_sections.size(); i++) {
        if (m_sections[i].name == std::string(name)) {
            m_sections[i].time += durationMs;
            m_sections[i].callCount++;
            return;
        }
    }

    ProfileSection section;
    section.name = name;
    section.time = durationMs;
    section.callCount = 1;
    m_sections.push_back(section);
}

void GPUProfiler::PrintResults() {
    OutputDebugStringA("\n=== GPU Profile Results ===\n");

    float totalTime = GetTotalTime();

    for (size_t i = 0; i < m_lastFrameSections.size(); i++) {
        const ProfileSection& section = m_lastFrameSections[i];
        float percentage = (totalTime > 0) ? (section.time / totalTime * 100.0f) : 0.0f;

        char buf[256];
        sprintf(buf, "  %s: %.2f ms (%d calls, %.1f%%)\n",
                section.name.c_str(), section.time, section.callCount, percentage);
        OutputDebugStringA(buf);
    }

    char buf[256];
    sprintf(buf, "  TOTAL: %.2f ms\n", totalTime);
    OutputDebugStringA(buf);

    OutputDebugStringA("===========================\n\n");
}

}
