#pragma once
#include "IRenderPass.h"

namespace Graphics {

class ShaderManager;
class GPUTimer;
class RenderFrame;

class TransparentPass : public IRenderPass {
public:
    TransparentPass(ShaderManager* shaderMgr, GPUTimer* timer);

    virtual const char* GetName() const override { return "TransparentPass"; }
    virtual RenderPassType GetType() const override { return PASS_TRANSPARENT; }
    virtual int GetPriority() const override { return 2; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void BeginPass() override;
    virtual void Execute() override;
    virtual void EndPass() override;

    void SetRenderFrame(RenderFrame* frame);
    RenderFrame* GetRenderFrame() const { return m_renderFrame; }

private:
    ShaderManager* m_shaderManager;
    GPUTimer* m_gpuTimer;
    RenderFrame* m_renderFrame;
    int m_gpuTimerIndex;
};

}