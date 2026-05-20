#include "stdafx.h"
#include "GeometryPass.h"
#include "ShaderManager.h"
#include "SpriteRenderer.h"
#include "GPUTimer.h"
#include "RenderContext.h"
#include "RenderFrame.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

GeometryPass::GeometryPass(ShaderManager* shaderMgr, SpriteRenderer* spriteRenderer, GPUTimer* timer)
    : IRenderPass("GeometryPass", PASS_GEOMETRY, 0)
    , m_shaderManager(shaderMgr)
    , m_spriteRenderer(spriteRenderer)
    , m_gpuTimer(timer)
    , m_renderFrame(NULL)
    , m_shaderID(SHADER_SPRITE_GBUFFER)
    , m_gpuTimerIndex(-1)
{
}

void GeometryPass::SetRenderFrame(RenderFrame* frame) {
    m_renderFrame = frame;
}

void GeometryPass::BeginPass() {
    if (!m_enabled) return;

    if (m_gpuTimer) {
        m_gpuTimerIndex = m_gpuTimer->StartTimer("GeometryPass");
    }

    BindGBuffer();
    ClearGBuffers();
}

void GeometryPass::Execute() {
    if (!m_enabled) return;
    if (!m_shaderManager || !m_spriteRenderer) return;

    ExecuteGeometry();
}

void GeometryPass::EndPass() {
    if (m_gpuTimer && m_gpuTimerIndex >= 0) {
        m_gpuTimer->EndTimer(m_gpuTimerIndex);
        m_gpuTimerIndex = -1;
    }
}

void GeometryPass::BindGBuffer() {
    if (!m_renderFrame) return;

    IDirect3DDevice9* device = m_spriteRenderer ? m_spriteRenderer->GetDevice() : NULL;
    if (!device) return;

    device->SetRenderTarget(0, m_renderFrame->GetGBufferPos());
    device->SetRenderTarget(1, m_renderFrame->GetGBufferNormal());
    device->SetRenderTarget(2, m_renderFrame->GetGBufferAlbedo());
    device->SetRenderTarget(3, m_renderFrame->GetGBufferSpec());
    device->SetDepthStencilSurface(m_renderFrame->GetGBufferDepth());
}

void GeometryPass::ClearGBuffers() {
    if (!m_spriteRenderer) return;

    IDirect3DDevice9* device = m_spriteRenderer->GetDevice();
    if (!device) return;

    device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);
}

void GeometryPass::ExecuteGeometry() {
    if (!m_shaderManager || !m_spriteRenderer) return;

    m_shaderManager->SetActiveShader(m_shaderID);

    LPDIRECT3DVERTEXBUFFER9 pVB = m_spriteRenderer->GetVertexBuffer();
    LPDIRECT3DINDEXBUFFER9 pIB = m_spriteRenderer->GetIndexBuffer();
    LPDIRECT3DVERTEXDECLARATION9 pDecl = m_spriteRenderer->GetVertexDeclaration();

    if (pVB && pIB && pDecl) {
        const D3DXMATRIX& viewProj = m_shaderManager->GetFrameViewProj();
        m_shaderManager->ExecuteQueue(pVB, pIB, pDecl, 32, &viewProj, m_spriteRenderer);
    }
}

}