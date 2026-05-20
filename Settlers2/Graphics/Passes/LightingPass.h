#pragma once
#include "IRenderPass.h"
#include <d3d9.h>

namespace Graphics {

class ShaderManager;
class GPUTimer;
class RenderFrame;

class LightingPass : public IRenderPass {
public:
    LightingPass(ShaderManager* shaderMgr, GPUTimer* timer);

    virtual const char* GetName() const override { return "LightingPass"; }
    virtual RenderPassType GetType() const override { return PASS_LIGHTING; }
    virtual int GetPriority() const override { return 1; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void BeginPass() override;
    virtual void Execute() override;
    virtual void EndPass() override;

    void SetRenderFrame(RenderFrame* frame);
    RenderFrame* GetRenderFrame() const { return m_renderFrame; }

    void SetDebugView(int view) { m_debugView = view; }
    int GetDebugView() const { return m_debugView; }

private:
    void UnbindGBuffer();
    void ApplyDeferredLighting();
    void RenderFullscreenQuad();

    ShaderManager* m_shaderManager;
    GPUTimer* m_gpuTimer;
    RenderFrame* m_renderFrame;

    int m_debugView;
    int m_gpuTimerIndex;
};

}