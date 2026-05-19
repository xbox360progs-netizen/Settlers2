#include "stdafx.h"
#include "RenderPassBase.h"
#include "RenderStateValidator.h"

#ifdef _DEBUG
#define PASS_LOG(msg, ...) \
    do { \
        char _buf[256]; \
        sprintf(_buf, "[PASS] %s: " msg "\n", m_name.c_str(), __VA_ARGS__); \
        ::OutputDebugStringA(_buf); \
    } while(0)
#else
#define PASS_LOG(...) ((void)0)
#endif

namespace Graphics {

static RenderStateValidator* g_validator = NULL;

void SetGlobalValidator(RenderStateValidator* validator) {
    g_validator = validator;
}

RenderPassBase::RenderPassBase(const char* name, RenderPassType type, int priority)
    : m_name(name ? name : "")
    , m_type(type)
    , m_priority(priority)
    , m_enabled(true)
    , m_passActive(false)
{
}

RenderPassBase::~RenderPassBase() {
}

void RenderPassBase::BeginPass() {
    if (m_passActive) {
        PASS_LOG("ERROR: Pass already active!");
        return;
    }
    
    PASS_LOG("BeginPass");
    
    m_passActive = true;
    OnBeginPass();
}

void RenderPassBase::EndPass() {
    if (!m_passActive) {
        PASS_LOG("ERROR: Pass not active!");
        return;
    }
    
    PASS_LOG("EndPass");
    
    OnEndPass();
    m_passActive = false;
}

void RenderPassBase::SetResources(int* readRTs, int readCount, int* writeRTs, int writeCount) {
    m_readRTs.clear();
    m_writeRTs.clear();
    
    for (int i = 0; i < readCount; i++) {
        m_readRTs.push_back(readRTs[i]);
    }
    for (int i = 0; i < writeCount; i++) {
        m_writeRTs.push_back(writeRTs[i]);
    }
}

void RenderPassBase::AddGPUEvent() {
#ifdef _DEBUG
    char marker[256];
    sprintf(marker, "=== PASS: %s ===", m_name.c_str());
    PASS_LOG("GPU Event: %s", marker);
#endif
}

void RenderPassBase::ValidateState() {
    PASS_LOG("Validating state");
    OnValidateState();
    
    if (g_validator) {
        g_validator->Validate(m_name.c_str());
    }
}

void ResetBlendState(IDirect3DDevice9* device) {
    if (!device) return;
    
    device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE);
    device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO);
    device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ALPHAREF, 0);
    device->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_ALWAYS);
    
    device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALL);
    device->SetRenderState(D3DRS_COLORWRITEENABLE1, D3DCOLORWRITEENABLE_ALL);
    device->SetRenderState(D3DRS_COLORWRITEENABLE2, D3DCOLORWRITEENABLE_ALL);
    device->SetRenderState(D3DRS_COLORWRITEENABLE3, D3DCOLORWRITEENABLE_ALL);
}

void ResetDepthState(IDirect3DDevice9* device) {
    if (!device) return;
    
    device->SetRenderState(D3DRS_ZENABLE, TRUE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL);
    device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    device->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
    device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    device->SetRenderState(D3DRS_STENCILMASK, 0xFFFFFFFF);
    device->SetRenderState(D3DRS_STENCILWRITEMASK, 0xFFFFFFFF);
}

void ResetRasterizerState(IDirect3DDevice9* device) {
    if (!device) return;
    
    device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
    device->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    device->SetRenderState(D3DRS_DEPTHBIAS, 0);
    device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
    
    device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
    device->SetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE);
}

void ResetSamplerStates(IDirect3DDevice9* device) {
    if (!device) return;
    
    for (DWORD i = 0; i < 16; i++) {
        device->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
        device->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        device->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
        device->SetSamplerState(i, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
        device->SetSamplerState(i, D3DSAMP_MAXMIPLEVEL, 0);
        device->SetSamplerState(i, D3DSAMP_MIPMAPLODBIAS, 0);
        device->SetSamplerState(i, D3DSAMP_BORDERCOLOR, 0);
    }
}

void ResetShaders(IDirect3DDevice9* device) {
    if (!device) return;
    
    device->SetVertexShader(NULL);
    device->SetPixelShader(NULL);
    
    for (DWORD i = 0; i < 16; i++) {
        device->SetStreamSourceFreq(i, 1);
        device->SetStreamSource(i, NULL, 0, 0);
    }
    
    device->SetVertexDeclaration(NULL);
}

void ResetAllRenderStates(IDirect3DDevice9* device) {
    ResetBlendState(device);
    ResetDepthState(device);
    ResetRasterizerState(device);
    ResetSamplerStates(device);
    ResetShaders(device);
    
    IDirect3DSurface9* nullRT = NULL;
    for (int i = 0; i < 4; i++) {
        device->SetRenderTarget(i, nullRT);
    }
    device->SetDepthStencilSurface(NULL);
    
    device->SetTexture(0, NULL);
    device->SetTexture(1, NULL);
    device->SetTexture(2, NULL);
    device->SetTexture(3, NULL);
}

void ValidateRenderTargetBindings(IDirect3DDevice9* device, const char* passName) {
#ifdef _DEBUG
    if (!device) return;
    
    IDirect3DSurface9* rt = NULL;
    device->GetRenderTarget(0, &rt);
    
    if (!rt) {
        char msg[256];
        sprintf(msg, "[VALIDATION] %s: No render target bound!\n", passName);
        ::OutputDebugStringA(msg);
    }
    
    if (rt) rt->Release();
#endif
}

void ValidateDepthState(const char* passName) {
#ifdef _DEBUG
    if (!passName) return;
#endif
}

void ValidateBlendState(const char* passName) {
#ifdef _DEBUG
    if (!passName) return;
#endif
}

void ValidateRasterizerState(const char* passName) {
#ifdef _DEBUG
    if (!passName) return;
#endif
}

}