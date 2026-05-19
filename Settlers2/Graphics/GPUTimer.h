#pragma once
#include <d3d9.h>
#include <vector>
#include <string>

namespace Graphics {

struct GPUTimerResult {
    const char* name;
    float startMs;
    float endMs;
    float durationMs;
    int frameIndex;
};

class GPUTimer {
public:
    GPUTimer();
    ~GPUTimer();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    int StartTimer(const char* name);
    void EndTimer(int timerIndex);

    void BeginFrame();
    void EndFrame();

    void ResolveQueries();
    const std::vector<GPUTimerResult>& GetResults() const { return m_results; }
    void ClearResults();

    bool IsReady() const { return m_ready; }

private:
    IDirect3DDevice9* m_pDevice;
    bool m_ready;
    int m_currentFrame;

    struct TimerData {
        IDirect3DQUERY9* pStartQuery;
        IDirect3DQUERY9* pEndQuery;
        const char* name;
        bool started;
        bool ended;
        UINT64 startData;
        UINT64 endData;
        float startMs;
        float endMs;
    };

    std::vector<TimerData> m_timers;
    std::vector<GPUTimerResult> m_results;
    IDirect3DQUERY9* m_pFrameStartQuery;
    IDirect3DQUERY9* m_pFrameEndQuery;
    UINT64 m_frameStartData;
    UINT64 m_frameEndData;
    float m_frameStartMs;
    float m_frameEndMs;
};

class GPUPerformanceCounters {
public:
    GPUPerformanceCounters();
    ~GPUPerformanceCounters();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void RecordDrawCall() { m_drawCalls++; }
    void RecordTriangleCount(int count) { m_triangles += count; }
    void RecordRTBind() { m_rtBinds++; }
    void RecordShaderSwitch() { m_shaderSwitches++; }
    void RecordTextureBind() { m_textureBinds++; }
    void RecordStateChange() { m_stateChanges++; }
    void RecordResolve() { m_resolves++; }

    void ResetCounters();

    struct FrameStats {
        int drawCalls;
        int triangles;
        int rtBinds;
        int shaderSwitches;
        int textureBinds;
        int stateChanges;
        int resolves;
        float frameTimeMs;
    };

    const FrameStats& GetCurrentStats() const { return m_currentStats; }
    const FrameStats& GetAverageStats() const { return m_averageStats; }

    void SetFrameTime(float ms) { m_currentStats.frameTimeMs = ms; }
    void UpdateAverage();

private:
    IDirect3DDevice9* m_pDevice;
    FrameStats m_currentStats;
    FrameStats m_averageStats;
    int m_frameCount;
    int m_sampleCount;
};

}