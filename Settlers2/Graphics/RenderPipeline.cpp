#include "stdafx.h"
#include "RenderPipeline.h"
#include "RenderPassBase.h"

#ifdef _DEBUG
#define PIPELINE_LOG(msg, ...) \
    do { \
        char _buf[256]; \
        sprintf(_buf, "[Pipeline] " msg "\n", __VA_ARGS__); \
        ::OutputDebugStringA(_buf); \
    } while(0)
#else
#define PIPELINE_LOG(...) ((void)0)
#endif

static RenderPipeline* g_pipeline = NULL;

namespace Graphics {

RenderPipeline::RenderPipeline()
    : m_device(NULL)
    , m_width(0)
    , m_height(0)
    , m_initialized(false)
    , m_frameActive(false)
    , m_validationEnabled(true)
    , m_gpuTimingEnabled(true)
{
}

RenderPipeline::~RenderPipeline() {
    Shutdown();
}

void RenderPipeline::Initialize(IDirect3DDevice9* device, int width, int height) {
    m_device = device;
    m_width = width;
    m_height = height;
    
    m_rtManager.Initialize(device, width, height);
    m_queueManager.Initialize(device);
    m_gpuTimer.Initialize(device);
    m_validator.EnableValidation(m_validationEnabled);
    
    SetGlobalRTManager(&m_rtManager);
    SetGlobalQueueManager(&m_queueManager);
    SetGlobalValidator(&m_validator);
    
    m_initialized = true;
    g_pipeline = this;
    
    PIPELINE_LOG("Initialized %dx%d", width, height);
}

void RenderPipeline::Shutdown() {
    m_passes.clear();
    m_passByType.clear();
    
    m_queueManager.Shutdown();
    m_rtManager.Shutdown();
    m_gpuTimer.Shutdown();
    
    SetGlobalRTManager(NULL);
    SetGlobalQueueManager(NULL);
    SetGlobalValidator(NULL);
    
    m_device = NULL;
    m_initialized = false;
    g_pipeline = NULL;
    
    PIPELINE_LOG("Shutdown complete");
}

void RenderPipeline::BeginFrame() {
    if (!m_initialized || m_frameActive) return;
    
    m_frameActive = true;
    m_timings.clear();
    
    m_rtManager.BeginFrame();
    m_queueManager.BeginFrame();
    m_gpuTimer.BeginFrame();
    
    ResetDeviceState();
    
    PIPELINE_LOG("BeginFrame");
}

void RenderPipeline::EndFrame() {
    if (!m_frameActive) return;
    
    m_gpuTimer.EndFrame();
    m_queueManager.EndFrame();
    m_rtManager.EndFrame();
    
    m_frameActive = false;
    
    PIPELINE_LOG("EndFrame");
}

void RenderPipeline::AddPass(IPipelinePass* pass) {
    if (!pass) return;
    
    m_passes.push_back(pass);
    m_passByType[pass->GetType()] = pass;
    
    PIPELINE_LOG("Added pass: %s", pass->GetName());
}

void RenderPipeline::RemovePass(const char* name) {
    for (size_t i = 0; i < m_passes.size(); i++) {
        if (strcmp(m_passes[i]->GetName(), name) == 0) {
            m_passByType.erase(m_passes[i]->GetType());
            m_passes.erase(m_passes.begin() + i);
            return;
        }
    }
}

void RenderPipeline::RemovePass(RenderPassType type) {
    auto it = m_passByType.find(type);
    if (it != m_passByType.end()) {
        for (size_t i = 0; i < m_passes.size(); i++) {
            if (m_passes[i]->GetType() == type) {
                m_passes.erase(m_passes.begin() + i);
                break;
            }
        }
        m_passByType.erase(it);
    }
}

void RenderPipeline::Execute() {
    if (!m_initialized || !m_frameActive) return;
    
    PIPELINE_LOG("Execute %d passes", (int)m_passes.size());
    
    for (size_t i = 0; i < m_passes.size(); i++) {
        ExecutePass(m_passes[i]);
    }
    
    m_queueManager.ExecuteAll();
}

void RenderPipeline::ExecutePass(IPipelinePass* pass) {
    if (!pass || !pass->IsEnabled()) return;
    
    int timerId = -1;
    if (m_gpuTimingEnabled) {
        timerId = m_gpuTimer.StartTimer(pass->GetName());
    }
    
    ResetDeviceState();
    pass->BeginPass(m_device);
    
    if (m_validationEnabled) {
        m_validator.Validate(pass->GetName());
    }
    
    pass->Execute(m_device);
    
    pass->EndPass(m_device);
    
    if (timerId >= 0) {
        m_gpuTimer.EndTimer(timerId);
        
        PassTiming timing;
        timing.name = pass->GetName();
        timing.durationMs = m_gpuTimer.GetResults().back().durationMs;
        timing.valid = true;
        m_timings.push_back(timing);
    }
}

void RenderPipeline::ResetDeviceState() {
    if (!m_device) return;
    
    ResetAllRenderStates(m_device);
}

const std::vector<PassTiming>& RenderPipeline::GetPassTimings() const {
    return m_timings;
}

void RenderPipeline::ClearTimings() {
    m_timings.clear();
}

void RenderPipeline::SetValidationEnabled(bool enabled) {
    m_validationEnabled = enabled;
    m_validator.EnableValidation(enabled);
}

void RenderPipeline::SetGPUTimingEnabled(bool enabled) {
    m_gpuTimingEnabled = enabled;
}

void RenderPipeline::OnResize(int width, int height) {
    m_width = width;
    m_height = height;
    
    if (m_initialized) {
        m_rtManager.Shutdown();
        m_rtManager.Initialize(m_device, width, height);
    }
}

RenderPipeline* GetGlobalPipeline() {
    return g_pipeline;
}

void SetGlobalPipeline(RenderPipeline* pipeline) {
    g_pipeline = pipeline;
}

}