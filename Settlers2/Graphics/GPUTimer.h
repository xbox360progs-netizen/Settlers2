#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <xtl.h>
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
        IDirect3DQuery9* pStartQuery;
        IDirect3DQuery9* pEndQuery;
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
    IDirect3DQuery9* m_pFrameStartQuery;
    IDirect3DQuery9* m_pFrameEndQuery;
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

    void RecordDrawCall() { m_currentStats.drawCalls++; }
    void RecordTriangleCount(int count) { m_currentStats.triangles += count; }
    void RecordRTBind() { m_currentStats.rtBinds++; }
    void RecordShaderSwitch() { m_currentStats.shaderSwitches++; }
    void RecordTextureBind() { m_currentStats.textureBinds++; }
    void RecordStateChange() { m_currentStats.stateChanges++; }
    void RecordResolve() { m_currentStats.resolves++; }

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