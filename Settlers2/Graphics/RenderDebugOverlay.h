#pragma once
#include <d3d9.h>
#include <vector>
#include <string>

namespace Graphics {

struct RenderStats {
    int drawCalls;
    int triangles;
    int vertices;
    int textureSwitches;
    int shaderSwitches;
    int stateChanges;
    int rtSwitches;

    float frameTime;
    float gpuTime;
    float cpuTime;

    int lightCount;
    int shadowMapCount;

    int batchCount;
    int instancedDraws;

    int spriteCount;
    int vbFlushes;

    void Reset() {
        drawCalls = 0;
        triangles = 0;
        vertices = 0;
        textureSwitches = 0;
        shaderSwitches = 0;
        stateChanges = 0;
        rtSwitches = 0;
        frameTime = 0;
        gpuTime = 0;
        cpuTime = 0;
        lightCount = 0;
        shadowMapCount = 0;
        batchCount = 0;
        instancedDraws = 0;
        spriteCount = 0;
        vbFlushes = 0;
    }
};

class RenderDebugOverlay {
public:
    RenderDebugOverlay();
    ~RenderDebugOverlay();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void RecordDrawCall(int vertexCount, int primitiveCount);
    void RecordTextureSwitch();
    void RecordShaderSwitch();
    void RecordStateChange();
    void RecordRenderTargetSwitch();
    void RecordLightCount(int count);
    void RecordShadowMapCount(int count);
    void RecordBatch(int vertexCount, bool instanced);
    void RecordSpriteCount(int count) { m_stats.spriteCount = count; }
    void RecordVBFlush() { m_stats.vbFlushes++; }

    void SetFrameTime(float ms) { m_stats.frameTime = ms; }
    void SetGPUTime(float ms) { m_stats.gpuTime = ms; }
    void SetCPUTime(float ms) { m_stats.cpuTime = ms; }

    const RenderStats& GetStats() const { return m_stats; }

    void RenderOverlay(int screenWidth, int screenHeight);

    void SetOverlayEnabled(bool enable) { m_overlayEnabled = enable; }
    bool IsOverlayEnabled() const { return m_overlayEnabled; }

    void SetShowFPS(bool show) { m_showFPS = show; }
    void SetShowDrawCalls(bool show) { m_showDrawCalls = show; }
    void SetShowTriangles(bool show) { m_showTriangles = show; }
    void SetShowLightCount(bool show) { m_showLightCount = show; }
    void SetShowMemory(bool show) { m_showMemory = show; }
    void SetShowGBuffer(bool show) { m_showGBuffer = show; }

    void DrawText(int x, int y, const char* text, DWORD color = 0xFFFFFFFF);

    void CaptureGBufferSnapshot(int gbufferIndex);
    void RenderGBufferView(int gbufferIndex, int screenWidth, int screenHeight);

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    bool m_overlayEnabled;
    bool m_showFPS;
    bool m_showDrawCalls;
    bool m_showTriangles;
    bool m_showLightCount;
    bool m_showMemory;
    bool m_showGBuffer;

    RenderStats m_stats;
    RenderStats m_lastFrameStats;

    float m_fps;
    float m_frameTimeAccum;
    int m_frameCount;

    std::vector<std::string> m_overlayLines;

    int m_lastGBufferIndex;
    bool m_gBufferCaptured;
    void* m_gBufferSnapshot;

    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_frameStart;

    void UpdateFPS(float deltaTime);
    void DrawOverlayBackground(int x, int y, int width, int height);
    void FormatStats();
};

class GPUProfiler {
public:
    GPUProfiler();
    ~GPUProfiler();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void BeginSection(const char* name);
    void EndSection();

    void BeginFrame();
    void EndFrame();

    float GetSectionTime(const char* name);
    float GetTotalTime();

    void SetEnabled(bool enable) { m_enabled = enable; }
    bool IsEnabled() const { return m_enabled; }

    void PrintResults();

    struct ProfileSection {
        std::string name;
        float time;
        int callCount;
    };

    const std::vector<ProfileSection>& GetSections() const { return m_sections; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    bool m_enabled;

    std::vector<ProfileSection> m_sections;
    std::vector<ProfileSection> m_lastFrameSections;

    struct SectionTimer {
        std::string name;
        LARGE_INTEGER startTime;
        float duration;
    };

    std::vector<SectionTimer> m_activeTimers;
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_frameStart;

    void StartTimer(const char* name);
    void StopTimer(const char* name);
    void RecordSection(const char* name, float durationMs);
};

}