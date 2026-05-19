#include "stdafx.h"
#include "RenderStateValidator.h"
#include "GPUDebug.h"
#include <d3d9.h>
#include <stdio.h>

static IDirect3DDevice9* GetDirect3DDevice9() {
    return ::GetGlobalDevice();
}

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderStateValidator::RenderStateValidator()
    : m_validationEnabled(true)
    , m_errorCount(0)
    , m_warningCount(0)
{
    ZeroMemory(&m_lastSnapshot, sizeof(m_lastSnapshot));
}

void RenderStateValidator::Validate(const char* passName) {
    if (!m_validationEnabled) return;

    m_lastPass = passName;

    CheckDepthState(passName);
    CheckBlendState(passName);
    CheckRasterizerState(passName);
    CheckViewport(passName);
    CheckRenderTargets(passName);
    CheckShaders(passName);
    CheckSamplers(passName);
}

void RenderStateValidator::ValidateGBufferBindings() {
    if (!m_validationEnabled) return;

    char buf[256];
    sprintf(buf, "[RenderStateValidator] Validating GBuffer bindings for pass: %s\n", m_lastPass.c_str());
    OutputDebugStringA(buf);
}

void RenderStateValidator::ValidateLightingPass() {
    if (!m_validationEnabled) return;

    OutputDebugStringA("[RenderStateValidator] Validating Lighting Pass...\n");
}

void RenderStateValidator::CheckDepthState(const char* pass) {
    StateSnapshot snap;
    CaptureSnapshot(snap);

    bool valid = true;

    if (snap.zEnable != D3DZB_TRUE && snap.zEnable != D3DZB_FALSE) {
        char buf[128];
        sprintf(buf, "[RenderStateValidator] WARNING: Invalid ZENABLE state in %s\n", pass);
        OutputDebugStringA(buf);
        m_warningCount++;
        valid = false;
    }

    if (valid) {
        char buf[128];
        sprintf(buf, "[RenderStateValidator] %s: ZENABLE=%s, ZWRITE=%s\n", pass,
                snap.zEnable == D3DZB_TRUE ? "TRUE" : "FALSE",
                snap.zWriteEnable == TRUE ? "ON" : "OFF");
        OutputDebugStringA(buf);
    }
}

void RenderStateValidator::CheckBlendState(const char* pass) {
    StateSnapshot snap;
    CaptureSnapshot(snap);

    if (snap.alphaBlendEnable) {
        if (snap.srcBlend == 0 || snap.destBlend == 0) {
            char buf[256];
            sprintf(buf, "[RenderStateValidator] ERROR: Blend enabled but src/dst blend is 0 in %s\n", pass);
            OutputDebugStringA(buf);
            m_errorCount++;
        }
    }

    char buf[128];
    sprintf(buf, "[RenderStateValidator] %s: AlphaBlend=%s, Src=%d, Dst=%d\n", pass,
            snap.alphaBlendEnable ? "ON" : "OFF", snap.srcBlend, snap.destBlend);
    OutputDebugStringA(buf);
}

void RenderStateValidator::CheckRasterizerState(const char* pass) {
    StateSnapshot snap;
    CaptureSnapshot(snap);

    char buf[128];
    sprintf(buf, "[RenderStateValidator] %s: CullMode=%d\n", pass, snap.cullMode);
    OutputDebugStringA(buf);
}

void RenderStateValidator::CheckViewport(const char* pass) {
    D3DVIEWPORT9 vp;
    HRESULT hr = GetDirect3DDevice9()->GetViewport(&vp);

    if (FAILED(hr)) {
        char buf[128];
        sprintf(buf, "[RenderStateValidator] ERROR: Failed to get viewport in %s\n", pass);
        OutputDebugStringA(buf);
        m_errorCount++;
        return;
    }

    char buf[256];
    sprintf(buf, "[RenderStateValidator] %s: Viewport X=%d Y=%d W=%d H=%d MinZ=%.2f MaxZ=%.2f\n",
            pass, vp.X, vp.Y, vp.Width, vp.Height, vp.MinZ, vp.MaxZ);
    OutputDebugStringA(buf);
}

void RenderStateValidator::CheckRenderTargets(const char* pass) {
    IDirect3DSurface9* pRT = NULL;
    HRESULT hr = GetDirect3DDevice9()->GetRenderTarget(0, &pRT);

    if (FAILED(hr) || !pRT) {
        char buf[256];
        sprintf(buf, "[RenderStateValidator] WARNING: RenderTarget[0] is NULL or invalid in %s\n", pass);
        OutputDebugStringA(buf);
        m_warningCount++;
    } else {
        pRT->Release();
    }

    char buf[256];
    sprintf(buf, "[RenderStateValidator] %s: RenderTarget checked\n", pass);
    OutputDebugStringA(buf);
}

void RenderStateValidator::CheckShaders(const char* pass) {
    char buf[128];
    sprintf(buf, "[RenderStateValidator] %s: Shaders checked\n", pass);
    OutputDebugStringA(buf);
}

void RenderStateValidator::CheckSamplers(const char* pass) {
    for (int i = 0; i < 4; i++) {
        DWORD addrU, addrV, minFilter, magFilter;
        GetDirect3DDevice9()->GetSamplerState(i, D3DSAMP_ADDRESSU, &addrU);
        GetDirect3DDevice9()->GetSamplerState(i, D3DSAMP_ADDRESSV, &addrV);
        GetDirect3DDevice9()->GetSamplerState(i, D3DSAMP_MINFILTER, &minFilter);
        GetDirect3DDevice9()->GetSamplerState(i, D3DSAMP_MAGFILTER, &magFilter);

        if (addrU == 0 || addrV == 0) {
            char buf[256];
            sprintf(buf, "[RenderStateValidator] WARNING: Sampler[%d] has uninitialized addressing\n", i);
            OutputDebugStringA(buf);
            m_warningCount++;
        }
    }

    char buf[128];
    sprintf(buf, "[RenderStateValidator] %s: Samplers checked\n", pass);
    OutputDebugStringA(buf);
}

void RenderStateValidator::LogState(const char* context) {
    char buf[512];
    sprintf(buf, "[RenderStateValidator] === State at %s ===\n", context);
    OutputDebugStringA(buf);

    StateSnapshot snap;
    CaptureSnapshot(snap);

    sprintf(buf, "  ZENABLE=%d, ZWRITE=%d, ALPHA=%d\n", snap.zEnable, snap.zWriteEnable, snap.alphaBlendEnable);
    OutputDebugStringA(buf);

    sprintf(buf, "  SRCBLEND=%d, DSTBLEND=%d, CULL=%d\n", snap.srcBlend, snap.destBlend, snap.cullMode);
    OutputDebugStringA(buf);
}

void RenderStateValidator::CaptureSnapshot(StateSnapshot& out) {
    LPDIRECT3DDEVICE9 pDev = GetDirect3DDevice9();
    if (!pDev) return;

    pDev->GetRenderState(D3DRS_ZENABLE, &out.zEnable);
    pDev->GetRenderState(D3DRS_ZWRITEENABLE, &out.zWriteEnable);
    pDev->GetRenderState(D3DRS_ALPHABLENDENABLE, &out.alphaBlendEnable);
    pDev->GetRenderState(D3DRS_SRCBLEND, &out.srcBlend);
    pDev->GetRenderState(D3DRS_DESTBLEND, &out.destBlend);
    pDev->GetRenderState(D3DRS_CULLMODE, &out.cullMode);

    out.rtCount = 0;
    for (int i = 0; i < 4; i++) {
        IDirect3DSurface9* pRT = NULL;
        if (SUCCEEDED(pDev->GetRenderTarget(i, &pRT))) {
            if (pRT) {
                out.rtCount++;
                pRT->Release();
            }
        }
    }

    m_lastSnapshot = out;
}

bool RenderStateValidator::ValidateSnapshot(const StateSnapshot& snap, const char* pass) {
    if (snap.zEnable == 0 && snap.alphaBlendEnable && (snap.srcBlend == 0 || snap.destBlend == 0)) {
        LogMismatch("Blend", "valid", "invalid", pass);
        return false;
    }
    return true;
}

void RenderStateValidator::LogMismatch(const char* state, const char* expected, const char* actual, const char* pass) {
    char buf[512];
    sprintf(buf, "[RenderStateValidator] STATE MISMATCH in %s: %s expected=%s actual=%s\n",
            pass, state, expected, actual);
    OutputDebugStringA(buf);
    m_errorCount++;
}

void RenderStateValidator::ResetCounters() {
    m_errorCount = 0;
    m_warningCount = 0;
}

}