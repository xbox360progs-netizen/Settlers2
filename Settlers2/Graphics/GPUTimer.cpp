#include "stdafx.h"
#include "GPUTimer.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

GPUTimer::GPUTimer()
    : m_pDevice(NULL)
    , m_ready(false)
    , m_currentFrame(0)
    , m_pFrameStartQuery(NULL)
    , m_pFrameEndQuery(NULL)
    , m_frameStartData(0)
    , m_frameEndData(0)
    , m_frameStartMs(0.0f)
    , m_frameEndMs(0.0f)
{
}

GPUTimer::~GPUTimer() {
    Shutdown();
}

void GPUTimer::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    if (!m_pDevice) return;

    HRESULT hr;

    hr = m_pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pFrameStartQuery);
    if (FAILED(hr)) {
        OutputDebugStringA("[GPUTimer] ERROR: Failed to create frame start query\n");
        return;
    }

    hr = m_pDevice->CreateQuery(D3DQUERYTYPE_EVENT, &m_pFrameEndQuery);
    if (FAILED(hr)) {
        OutputDebugStringA("[GPUTimer] ERROR: Failed to create frame end query\n");
        return;
    }

    m_ready = true;
    OutputDebugStringA("[GPUTimer] Initialized\n");
}

void GPUTimer::Shutdown() {
    for (size_t i = 0; i < m_timers.size(); i++) {
        if (m_timers[i].pStartQuery) {
            m_timers[i].pStartQuery->Release();
            m_timers[i].pStartQuery = NULL;
        }
        if (m_timers[i].pEndQuery) {
            m_timers[i].pEndQuery->Release();
            m_timers[i].pEndQuery = NULL;
        }
    }
    m_timers.clear();

    if (m_pFrameStartQuery) {
        m_pFrameStartQuery->Release();
        m_pFrameStartQuery = NULL;
    }
    if (m_pFrameEndQuery) {
        m_pFrameEndQuery->Release();
        m_pFrameEndQuery = NULL;
    }

    m_ready = false;
}

int GPUTimer::StartTimer(const char* name) {
    if (!m_ready) return -1;

    TimerData timer;
    timer.name = name;
    timer.started = true;
    timer.ended = false;
    timer.startData = 0;
    timer.endData = 0;
    timer.startMs = 0.0f;
    timer.endMs = 0.0f;

    HRESULT hr = m_pDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &timer.pStartQuery);
    if (FAILED(hr)) return -1;

    hr = m_pDevice->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &timer.pEndQuery);
    if (FAILED(hr)) {
        timer.pStartQuery->Release();
        return -1;
    }

    timer.pStartQuery->Issue(D3DISSUE_END);

    int index = (int)m_timers.size();
    m_timers.push_back(timer);

    return index;
}

void GPUTimer::EndTimer(int timerIndex) {
    if (!m_ready || timerIndex < 0 || timerIndex >= (int)m_timers.size()) return;

    if (m_timers[timerIndex].pEndQuery) {
        m_timers[timerIndex].pEndQuery->Issue(D3DISSUE_END);
    }
    m_timers[timerIndex].ended = true;
}

void GPUTimer::BeginFrame() {
    if (!m_ready || !m_pFrameStartQuery) return;

    m_pFrameStartQuery->Issue(D3DISSUE_END);
    m_currentFrame++;
}

void GPUTimer::EndFrame() {
    if (!m_ready || !m_pFrameEndQuery) return;

    m_pFrameEndQuery->Issue(D3DISSUE_END);
}

void GPUTimer::ResolveQueries() {
    if (!m_ready) return;

    for (size_t i = 0; i < m_timers.size(); i++) {
        TimerData& timer = m_timers[i];

        if (timer.started && timer.pStartQuery) {
            while (timer.pStartQuery->GetData(&timer.startData, sizeof(timer.startData), 0) == S_FALSE) {
            }
        }

        if (timer.ended && timer.pEndQuery) {
            while (timer.pEndQuery->GetData(&timer.endData, sizeof(timer.endData), 0) == S_FALSE) {
            }
        }

        if (timer.startData > 0 && timer.endData > 0) {
            float startMs = (float)(timer.startData / 10000.0);
            float endMs = (float)(timer.endData / 10000.0);
            float durationMs = endMs - startMs;

            if (durationMs > 2.0f) {
                char buf[256];
                sprintf(buf, "[GPUTimer] SPIKE: %s took %.2fms (>2ms threshold)\n", timer.name, durationMs);
                OutputDebugStringA(buf);
            }

            GPUTimerResult result;
            result.name = timer.name;
            result.startMs = startMs;
            result.endMs = endMs;
            result.durationMs = durationMs;
            result.frameIndex = m_currentFrame;
            m_results.push_back(result);

            timer.startData = 0;
            timer.endData = 0;
        }
    }
}

void GPUTimer::ClearResults() {
    m_results.clear();
}

GPUPerformanceCounters::GPUPerformanceCounters()
    : m_pDevice(NULL)
    , m_frameCount(0)
    , m_sampleCount(60)
{
    ZeroMemory(&m_currentStats, sizeof(m_currentStats));
    ZeroMemory(&m_averageStats, sizeof(m_averageStats));
}

GPUPerformanceCounters::~GPUPerformanceCounters() {
    Shutdown();
}

void GPUPerformanceCounters::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    OutputDebugStringA("[GPUPerformanceCounters] Initialized\n");
}

void GPUPerformanceCounters::Shutdown() {
    m_pDevice = NULL;
}

void GPUPerformanceCounters::BeginFrame() {
    ResetCounters();
}

void GPUPerformanceCounters::EndFrame() {
    UpdateAverage();
}

void GPUPerformanceCounters::ResetCounters() {
    ZeroMemory(&m_currentStats, sizeof(m_currentStats));
}

void GPUPerformanceCounters::UpdateAverage() {
    m_frameCount++;

    m_averageStats.drawCalls += m_currentStats.drawCalls;
    m_averageStats.triangles += m_currentStats.triangles;
    m_averageStats.rtBinds += m_currentStats.rtBinds;
    m_averageStats.shaderSwitches += m_currentStats.shaderSwitches;
    m_averageStats.textureBinds += m_currentStats.textureBinds;
    m_averageStats.stateChanges += m_currentStats.stateChanges;
    m_averageStats.resolves += m_currentStats.resolves;

    if (m_frameCount >= m_sampleCount) {
        float invCount = 1.0f / m_sampleCount;
        m_averageStats.drawCalls = (int)(m_averageStats.drawCalls * invCount);
        m_averageStats.triangles = (int)(m_averageStats.triangles * invCount);
        m_averageStats.rtBinds = (int)(m_averageStats.rtBinds * invCount);
        m_averageStats.shaderSwitches = (int)(m_averageStats.shaderSwitches * invCount);
        m_averageStats.textureBinds = (int)(m_averageStats.textureBinds * invCount);
        m_averageStats.stateChanges = (int)(m_averageStats.stateChanges * invCount);
        m_averageStats.resolves = (int)(m_averageStats.resolves * invCount);

        m_frameCount = 0;
    }
}

}