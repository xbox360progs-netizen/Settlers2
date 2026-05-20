#include "stdafx.h"
#include "RenderContext.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderContext::RenderContext()
    : m_device(NULL)
    , m_dirtyFlags(DIRTY_ALL)
    , m_currentShader(SHADER_INVALID)
    , m_currentTextureSlot(0)
{
    for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
        m_currentTextures[i] = NULL;
    }
}

RenderContext::~RenderContext() {
    Shutdown();
}

void RenderContext::Initialize(LPDIRECT3DDEVICE9 device) {
    m_device = device;
    ResetToDefaults();
    OutputDebugStringA("[RenderContext] Initialized\n");
}

void RenderContext::Shutdown() {
    for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
        m_currentTextures[i] = NULL;
    }
    m_renderTargets.clear();
    m_device = NULL;
    m_dirtyFlags = DIRTY_ALL;
}

void RenderContext::BeginFrame() {
    m_dirtyFlags = DIRTY_ALL;
}

void RenderContext::EndFrame() {
}

void RenderContext::SetShader(ShaderID shader) {
    if (m_currentShader != shader) {
        m_currentShader = shader;
        m_dirtyFlags |= DIRTY_SHADER;
    }
}

void RenderContext::SetTexture(int slot, LPDIRECT3DBASETEXTURE9 texture) {
    if (slot >= 0 && slot < MAX_TEXTURE_SLOTS) {
        if (m_currentTextures[slot] != texture) {
            m_currentTextures[slot] = texture;
            m_dirtyFlags |= DIRTY_TEXTURES;
        }
    }
}

LPDIRECT3DBASETEXTURE9 RenderContext::GetCurrentTexture(int slot) const {
    if (slot >= 0 && slot < MAX_TEXTURE_SLOTS) {
        return m_currentTextures[slot];
    }
    return NULL;
}

void RenderContext::SetBlendState(const BlendState& state) {
    if (m_blendState.enabled != state.enabled ||
        m_blendState.srcBlend != state.srcBlend ||
        m_blendState.destBlend != state.destBlend) {
        m_blendState = state;
        m_dirtyFlags |= DIRTY_BLEND;
    }
}

void RenderContext::SetDepthState(const DepthState& state) {
    if (m_depthState.zEnable != state.zEnable ||
        m_depthState.zWriteEnable != state.zWriteEnable ||
        m_depthState.zFunc != state.zFunc) {
        m_depthState = state;
        m_dirtyFlags |= DIRTY_DEPTH;
    }
}

void RenderContext::SetRasterizerState(const RasterizerState& state) {
    if (m_rasterizerState.cullMode != state.cullMode ||
        m_rasterizerState.wireframe != state.wireframe) {
        m_rasterizerState = state;
        m_dirtyFlags |= DIRTY_RASTERIZER;
    }
}

const SamplerState& RenderContext::GetSamplerState(int slot) const {
    static SamplerState defaultState;
    if (slot >= 0 && slot < MAX_TEXTURE_SLOTS) {
        return m_samplerStates[slot];
    }
    return defaultState;
}

void RenderContext::SetSamplerState(int slot, const SamplerState& state) {
    if (slot >= 0 && slot < MAX_TEXTURE_SLOTS) {
        m_samplerStates[slot] = state;
        m_dirtyFlags |= DIRTY_SAMPLERS;
    }
}

void RenderContext::SetViewport(const Viewport& viewport) {
    if (m_viewport.x != viewport.x ||
        m_viewport.y != viewport.y ||
        m_viewport.width != viewport.width ||
        m_viewport.height != viewport.height) {
        m_viewport = viewport;
        m_dirtyFlags |= DIRTY_VIEWPORT;
    }
}

const RenderTargetBinding& RenderContext::GetRenderTarget(int index) const {
    static RenderTargetBinding defaultBinding;
    if (index >= 0 && index < (int)m_renderTargets.size()) {
        return m_renderTargets[index];
    }
    return defaultBinding;
}

void RenderContext::InvalidateAll() {
    m_dirtyFlags = DIRTY_ALL;
}

void RenderContext::ResetToDefaults() {
    m_currentShader = SHADER_INVALID;
    m_currentTextureSlot = 0;

    for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
        m_currentTextures[i] = NULL;
        m_samplerStates[i] = SamplerState();
    }

    m_blendState = BlendState();
    m_depthState = DepthState();
    m_rasterizerState = RasterizerState();
    m_viewport = Viewport();

    m_renderTargets.clear();
    RenderTargetBinding rt;
    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        rt.index = i;
        rt.surface = NULL;
        rt.isDepth = false;
        m_renderTargets.push_back(rt);
    }

    m_dirtyFlags = DIRTY_ALL;
}

void RenderContext::ApplyCurrentState(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    if (m_dirtyFlags & DIRTY_SHADER) {
        ApplyShader(device);
    }
    if (m_dirtyFlags & DIRTY_TEXTURES) {
        ApplyTextures(device);
    }
    if (m_dirtyFlags & DIRTY_BLEND) {
        ApplyBlendState(device);
    }
    if (m_dirtyFlags & DIRTY_DEPTH) {
        ApplyDepthState(device);
    }
    if (m_dirtyFlags & DIRTY_RASTERIZER) {
        ApplyRasterizerState(device);
    }
    if (m_dirtyFlags & DIRTY_SAMPLERS) {
        ApplySamplerStates(device);
    }
    if (m_dirtyFlags & DIRTY_VIEWPORT) {
        ApplyViewport(device);
    }
    if (m_dirtyFlags & DIRTY_RENDERTARGETS) {
        ApplyRenderTargets(device);
    }

    m_dirtyFlags = DIRTY_NONE;
}

void RenderContext::ApplyShader(LPDIRECT3DDEVICE9 device) {
    if (m_dirtyFlags & DIRTY_SHADER) {
        OutputDebugStringA("[RenderContext] Applying shader state\n");
    }
}

void RenderContext::ApplyTextures(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
        if (m_currentTextures[i]) {
            device->SetTexture(i, m_currentTextures[i]);
        }
    }
}

void RenderContext::ApplyBlendState(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    device->SetRenderState(D3DRS_ALPHABLENDENABLE, m_blendState.enabled ? TRUE : FALSE);

    if (m_blendState.enabled) {
        device->SetRenderState(D3DRS_SRCBLEND, m_blendState.srcBlend);
        device->SetRenderState(D3DRS_DESTBLEND, m_blendState.destBlend);
    }

    if (m_blendState.alphaFunc != D3DCMP_ALWAYS) {
        device->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
        device->SetRenderState(D3DRS_ALPHAFUNC, m_blendState.alphaFunc);
        device->SetRenderState(D3DRS_ALPHAREF, m_blendState.alphaRef);
    }
}

void RenderContext::ApplyDepthState(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    device->SetRenderState(D3DRS_ZENABLE, m_depthState.zEnable ? TRUE : FALSE);
    device->SetRenderState(D3DRS_ZWRITEENABLE, m_depthState.zWriteEnable ? TRUE : FALSE);
    device->SetRenderState(D3DRS_ZFUNC, m_depthState.zFunc);
}

void RenderContext::ApplyRasterizerState(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    device->SetRenderState(D3DRS_CULLMODE, m_rasterizerState.cullMode);
    device->SetRenderState(D3DRS_FILLMODE, m_rasterizerState.wireframe ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
}

void RenderContext::ApplySamplerStates(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    for (int i = 0; i < MAX_TEXTURE_SLOTS; i++) {
        device->SetSamplerState(i, D3DSAMP_MAGFILTER, m_samplerStates[i].filter);
        device->SetSamplerState(i, D3DSAMP_MINFILTER, m_samplerStates[i].filter);
        device->SetSamplerState(i, D3DSAMP_MIPFILTER, m_samplerStates[i].filter);
        device->SetSamplerState(i, D3DSAMP_ADDRESSU, m_samplerStates[i].addressU);
        device->SetSamplerState(i, D3DSAMP_ADDRESSV, m_samplerStates[i].addressV);
    }
}

void RenderContext::ApplyViewport(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    D3DVIEWPORT9 vp;
    vp.X = m_viewport.x;
    vp.Y = m_viewport.y;
    vp.Width = m_viewport.width;
    vp.Height = m_viewport.height;
    vp.MinZ = m_viewport.minZ;
    vp.MaxZ = m_viewport.maxZ;

    device->SetViewport(&vp);
}

void RenderContext::ApplyRenderTargets(LPDIRECT3DDEVICE9 device) {
    if (!device) return;

    for (int i = 0; i < MAX_RENDER_TARGETS; i++) {
        if (i < (int)m_renderTargets.size()) {
            const RenderTargetBinding& rt = m_renderTargets[i];
            if (rt.surface) {
                device->SetRenderTarget(rt.index, static_cast<IDirect3DSurface9*>(rt.surface));
            }
        }
    }
}

}