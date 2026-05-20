#include "stdafx.h"
#include "LightingPass.h"
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

LightingPass::LightingPass(ShaderManager* shaderMgr, GPUTimer* timer)
    : IRenderPass("LightingPass", PASS_LIGHTING, 1)
    , m_shaderManager(shaderMgr)
    , m_gpuTimer(timer)
    , m_renderFrame(NULL)
    , m_debugView(0)
    , m_gpuTimerIndex(-1)
{
}

void LightingPass::SetRenderFrame(RenderFrame* frame) {
    m_renderFrame = frame;
}

void LightingPass::BeginPass() {
    if (!m_enabled) return;

    if (m_gpuTimer) {
        m_gpuTimerIndex = m_gpuTimer->StartTimer("LightingPass");
    }

    UnbindGBuffer();
}

void LightingPass::Execute() {
    if (!m_enabled) return;
    if (!m_shaderManager) return;

    ApplyDeferredLighting();
}

void LightingPass::EndPass() {
    if (m_gpuTimer && m_gpuTimerIndex >= 0) {
        m_gpuTimer->EndTimer(m_gpuTimerIndex);
        m_gpuTimerIndex = -1;
    }
}

void LightingPass::UnbindGBuffer() {
    if (!m_renderFrame || !m_shaderManager) return;

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    device->SetRenderTarget(0, m_renderFrame->GetBackBuffer());
    device->SetRenderTarget(1, NULL);
    device->SetRenderTarget(2, NULL);
    device->SetRenderTarget(3, NULL);
    device->SetDepthStencilSurface(m_renderFrame->GetGBufferDepth());
}

void LightingPass::ApplyDeferredLighting() {
    if (!m_shaderManager) return;

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

    m_shaderManager->SetActiveShader(SHADER_DEFERRED_LIGHTING);
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);

    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    RenderFullscreenQuad();

    m_shaderManager->EndPass();
    m_shaderManager->EndShader();
}

void LightingPass::RenderFullscreenQuad() {
    if (!m_shaderManager) return;

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    D3DXVECTOR3 vertices[] = {
        D3DXVECTOR3(-1.0f, -1.0f, 0.0f),
        D3DXVECTOR3(1.0f, -1.0f, 0.0f),
        D3DXVECTOR3(-1.0f, 1.0f, 0.0f),
        D3DXVECTOR3(1.0f, 1.0f, 0.0f)
    };

    WORD indices[] = { 0, 1, 2, 1, 3, 2 };

    device->DrawIndexedPrimitiveUP(
        D3DPT_TRIANGLELIST,
        0,
        4,
        2,
        indices,
        D3DFMT_INDEX16,
        vertices,
        sizeof(D3DXVECTOR3)
    );
}

}