#include "stdafx.h"
#include "TransparentPass.h"
#include "ShaderManager.h"
#include "GPUTimer.h"
#include "RenderContext.h"
#include "RenderFrame.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

TransparentPass::TransparentPass(ShaderManager* shaderMgr, GPUTimer* timer)
    : IRenderPass("TransparentPass", PASS_TRANSPARENT, 2)
    , m_shaderManager(shaderMgr)
    , m_gpuTimer(timer)
    , m_renderFrame(NULL)
    , m_gpuTimerIndex(-1)
{
}

void TransparentPass::SetRenderFrame(RenderFrame* frame) {
    m_renderFrame = frame;
}

void TransparentPass::BeginPass() {
    if (!m_enabled) return;

    if (m_gpuTimer) {
        m_gpuTimerIndex = m_gpuTimer->StartTimer("TransparentPass");
    }

    if (!m_shaderManager) return;

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    if (m_renderFrame) {
        device->SetRenderTarget(0, m_renderFrame->GetBackBuffer());
    }

    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
}

void TransparentPass::Execute() {
    if (!m_enabled) return;
    if (!m_shaderManager || !m_renderFrame) return;

    m_shaderManager->SetActiveShader(SHADER_SPRITE);
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);
}

void TransparentPass::EndPass() {
    if (m_shaderManager) {
        m_shaderManager->EndPass();
        m_shaderManager->EndShader();
    }

    if (m_gpuTimer && m_gpuTimerIndex >= 0) {
        m_gpuTimer->EndTimer(m_gpuTimerIndex);
        m_gpuTimerIndex = -1;
    }
}

}