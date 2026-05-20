#include "stdafx.h"
#include "Passes.h"
#include "RenderFrame.h"

namespace Graphics {

void GeometryPass::Execute() {
    if (!m_shaderManager || !m_spriteRenderer) return;

    int timerIdx = -1;
    if (m_gpuTimer) {
        timerIdx = m_gpuTimer->StartTimer("GeometryPass");
    }

    IDirect3DDevice9* device = m_spriteRenderer->GetDevice();
    if (!device) return;

    device->SetRenderTarget(0, m_gBufferPos);
    device->SetRenderTarget(1, m_gBufferNormal);
    device->SetRenderTarget(2, m_gBufferAlbedo);
    device->SetRenderTarget(3, m_gBufferSpec);
    device->SetDepthStencilSurface(m_gBufferDepth);

    device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

    m_shaderManager->SetActiveShader(SHADER_SPRITE_GBUFFER);

    LPDIRECT3DVERTEXBUFFER9 pVB = m_spriteRenderer->GetVertexBuffer();
    LPDIRECT3DINDEXBUFFER9 pIB = m_spriteRenderer->GetIndexBuffer();
    LPDIRECT3DVERTEXDECLARATION9 pDecl = m_spriteRenderer->GetVertexDeclaration();

    if (pVB && pIB && pDecl) {
        const D3DXMATRIX& viewProj = m_shaderManager->GetFrameViewProj();
        m_shaderManager->ExecuteQueue(pVB, pIB, pDecl, 32, &viewProj, m_spriteRenderer);
    }


    if (m_gpuTimer && timerIdx >= 0) {
        m_gpuTimer->EndTimer(timerIdx);
    }
}

void LightingPass::Execute() {
    if (!m_shaderManager) return;

    int timerIdx = -1;
    if (m_gpuTimer) {
        timerIdx = m_gpuTimer->StartTimer("LightingPass");
    }

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    device->SetRenderTarget(0, m_backBuffer);
    device->SetRenderTarget(1, NULL);
    device->SetRenderTarget(2, NULL);
    device->SetRenderTarget(3, NULL);
    device->SetDepthStencilSurface(m_gBufferDepth);

    device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);

    m_shaderManager->SetActiveShader(SHADER_DEFERRED_LIGHTING);
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);

    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);

    m_shaderManager->EndPass();
    m_shaderManager->EndShader();


    if (m_gpuTimer && timerIdx >= 0) {
        m_gpuTimer->EndTimer(timerIdx);
    }
}

void AlphaTestPass::Execute() {
    if (!m_shaderManager) return;

    int timerIdx = -1;
    if (m_gpuTimer) {
        timerIdx = m_gpuTimer->StartTimer("AlphaTestPass");
    }

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    m_shaderManager->SetActiveShader(SHADER_SPRITE);
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);

    device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    device->SetRenderState(D3DRS_ALPHAREF, 128);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    m_shaderManager->EndPass();
    m_shaderManager->EndShader();


    if (m_gpuTimer && timerIdx >= 0) {
        m_gpuTimer->EndTimer(timerIdx);
    }
}

void TransparentPass::Execute() {
    if (!m_shaderManager) return;

    int timerIdx = -1;
    if (m_gpuTimer) {
        timerIdx = m_gpuTimer->StartTimer("TransparentPass");
    }

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    m_shaderManager->SetActiveShader(SHADER_SPRITE);
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);

    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    m_shaderManager->EndPass();
    m_shaderManager->EndShader();


    if (m_gpuTimer && timerIdx >= 0) {
        m_gpuTimer->EndTimer(timerIdx);
    }
}

void UIPass::Execute() {
    if (!m_shaderManager) return;

    int timerIdx = -1;
    if (m_gpuTimer) {
        timerIdx = m_gpuTimer->StartTimer("UIPass");
    }

    IDirect3DDevice9* device = m_shaderManager->GetDevice();
    if (!device) return;

    m_shaderManager->SetActiveShader(SHADER_SPRITE);
    m_shaderManager->BeginShader();
    m_shaderManager->BeginPass(0);

    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_ZENABLE, FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);

    m_shaderManager->EndPass();
    m_shaderManager->EndShader();


    if (m_gpuTimer && timerIdx >= 0) {
        m_gpuTimer->EndTimer(timerIdx);
    }
}

void PostFXPass::Execute() {
    int timerIdx = -1;
    if (m_gpuTimer) {
        timerIdx = m_gpuTimer->StartTimer("PostFXPass");
    }

    for (size_t i = 0; i < m_effects.size(); i++) {
        const PostFXCommand& cmd = m_effects[i];
        switch (cmd.type) {
            case PostFXCommand::POSTFX_BLOOM:
                break;
            case PostFXCommand::POSTFX_SSAO:
                break;
            case PostFXCommand::POSTFX_FOG:
                break;
            case PostFXCommand::POSTFX_TONEMAP:
                break;
            case PostFXCommand::POSTFX_COLORGRADE:
                break;
        }
    }

    if (m_gpuTimer && timerIdx >= 0) {
        m_gpuTimer->EndTimer(timerIdx);
    }
}

void PostFXPass::AddEffect(PostFXCommand::PostFXType type, float intensity, const float* params) {
    PostFXCommand cmd;
    cmd.type = type;
    cmd.intensity = intensity;
    if (params) {
        for (int i = 0; i < 4; i++) {
            cmd.params[i] = params[i];
        }
    }
    m_effects.push_back(cmd);
}

}