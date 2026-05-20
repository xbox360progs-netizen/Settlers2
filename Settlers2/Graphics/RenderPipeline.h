#pragma once
#include "RenderPassBase.h"
#include "RenderTargetManager.h"
#include "CommandQueueManager.h"
#include "GPUTimer.h"
#include "RenderStateValidator.h"
#include <vector>
#include <map>

namespace Graphics {

class IPipelinePass {
public:
    virtual ~IPipelinePass() {}
    
    virtual void BeginPass(IDirect3DDevice9* device) = 0;
    virtual void Execute(IDirect3DDevice9* device) = 0;
    virtual void EndPass(IDirect3DDevice9* device) = 0;
    
    virtual const char* GetName() const = 0;
    virtual RenderPassType GetType() const = 0;
    virtual bool IsEnabled() const = 0;
    virtual void SetEnabled(bool enabled) = 0;
};

struct PassTiming {
    const char* name;
    float startMs;
    float endMs;
    float durationMs;
    bool valid;
    
    PassTiming() : name(NULL), startMs(0), endMs(0), durationMs(0), valid(false) {}
};

class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();
    
    void Initialize(IDirect3DDevice9* device, int width, int height);
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void AddPass(IPipelinePass* pass);
    void RemovePass(const char* name);
    void RemovePass(RenderPassType type);
    
    void Execute();
    
    RenderTargetManager* GetRTManager() { return &m_rtManager; }
    CommandQueueManager* GetQueueManager() { return &m_queueManager; }
    GPUTimer* GetGPUTimer() { return &m_gpuTimer; }
    RenderStateValidator* GetValidator() { return &m_validator; }
    
    const std::vector<PassTiming>& GetPassTimings() const;
    void ClearTimings();
    
    void SetValidationEnabled(bool enabled);
    void SetGPUTimingEnabled(bool enabled);
    
    void OnResize(int width, int height);
    
private:
    IDirect3DDevice9* m_device;
    int m_width;
    int m_height;
    
    std::vector<IPipelinePass*> m_passes;
    std::map<RenderPassType, IPipelinePass*> m_passByType;
    
    RenderTargetManager m_rtManager;
    CommandQueueManager m_queueManager;
    GPUTimer m_gpuTimer;
    RenderStateValidator m_validator;
    
    std::vector<PassTiming> m_timings;
    
    bool m_initialized;
    bool m_frameActive;
    bool m_validationEnabled;
    bool m_gpuTimingEnabled;
    
    void ExecutePass(IPipelinePass* pass);
    void ResetDeviceState();
};

RenderPipeline* GetGlobalPipeline();
void SetGlobalPipeline(RenderPipeline* pipeline);

}