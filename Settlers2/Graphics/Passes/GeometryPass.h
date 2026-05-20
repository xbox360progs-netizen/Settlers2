#pragma once
#include "IRenderPass.h"
#include <d3d9.h>

namespace Graphics {

class ShaderManager;
class SpriteRenderer;
class GPUTimer;
class RenderFrame;

class GeometryPass : public IRenderPass {
public:
    GeometryPass(ShaderManager* shaderMgr, SpriteRenderer* spriteRenderer, GPUTimer* timer);

    virtual const char* GetName() const override { return "GeometryPass"; }
    virtual RenderPassType GetType() const override { return PASS_GEOMETRY; }
    virtual int GetPriority() const override { return 0; }
    virtual bool IsEnabled() const override { return m_enabled; }
    virtual void SetEnabled(bool enabled) override { m_enabled = enabled; }

    virtual void BeginPass() override;
    virtual void Execute() override;
    virtual void EndPass() override;

    void SetRenderFrame(RenderFrame* frame);
    RenderFrame* GetRenderFrame() const { return m_renderFrame; }

    void SetShader(ShaderID shaderID) { m_shaderID = shaderID; }
    ShaderID GetShader() const { return m_shaderID; }

private:
    void BindGBuffer();
    void ClearGBuffers();
    void ExecuteGeometry();

    ShaderManager* m_shaderManager;
    SpriteRenderer* m_spriteRenderer;
    GPUTimer* m_gpuTimer;
    RenderFrame* m_renderFrame;

    ShaderID m_shaderID;

    int m_gpuTimerIndex;
};

}