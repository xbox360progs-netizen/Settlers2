#pragma once
#include <d3d9.h>
#include <d3dx9.h>
#include <vector>
#include "ShaderManager.h"

namespace Graphics {

struct BlendState {
    bool enabled;
    D3DBLEND srcBlend;
    D3DBLEND destBlend;
    D3DCMPFUNC alphaFunc;
    BYTE alphaRef;

    BlendState()
        : enabled(false), srcBlend(D3DBLEND_ONE), destBlend(D3DBLEND_ZERO),
          alphaFunc(D3DCMP_ALWAYS), alphaRef(0) {}
};

struct DepthState {
    bool zEnable;
    bool zWriteEnable;
    D3DCMPFUNC zFunc;

    DepthState()
        : zEnable(true), zWriteEnable(true), zFunc(D3DCMP_LESS) {}
};

struct RasterizerState {
    D3DCULL cullMode;
    bool wireframe;

    RasterizerState()
        : cullMode(D3DCULL_NONE), wireframe(false) {}
};

struct SamplerState {
    D3DTEXTUREFILTERTYPE filter;
    D3DTEXTUREADDRESS addressU;
    D3DTEXTUREADDRESS addressV;

    SamplerState()
        : filter(D3DTEXF_LINEAR), addressU(D3DTADDRESS_WRAP), addressV(D3DTADDRESS_WRAP) {}
};

struct Viewport {
    int x, y, width, height;
    float minZ, maxZ;

    Viewport()
        : x(0), y(0), width(1280), height(720), minZ(0.0f), maxZ(1.0f) {}
};

struct RenderTargetBinding {
    int index;
    void* surface;
    bool isDepth;

    RenderTargetBinding()
        : index(-1), surface(nullptr), isDepth(false) {}
};

class RenderContext {
public:
    RenderContext();
    ~RenderContext();

    void Initialize(LPDIRECT3DDEVICE9 device);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    bool IsDirty() const { return m_dirtyFlags != 0; }
    void ClearDirty() { m_dirtyFlags = 0; }

    ShaderID GetCurrentShader() const { return m_currentShader; }
    void SetShader(ShaderID shader);

    int GetCurrentTextureSlot() const { return m_currentTextureSlot; }
    LPDIRECT3DBASETEXTURE9 GetCurrentTexture(int slot) const;
    void SetTexture(int slot, LPDIRECT3DBASETEXTURE9 texture);

    const BlendState& GetBlendState() const { return m_blendState; }
    void SetBlendState(const BlendState& state);

    const DepthState& GetDepthState() const { return m_depthState; }
    void SetDepthState(const DepthState& state);

    const RasterizerState& GetRasterizerState() const { return m_rasterizerState; }
    void SetRasterizerState(const RasterizerState& state);

    const SamplerState& GetSamplerState(int slot) const;
    void SetSamplerState(int slot, const SamplerState& state);

    const Viewport& GetViewport() const { return m_viewport; }
    void SetViewport(const Viewport& viewport);

    const RenderTargetBinding& GetRenderTarget(int index) const;
    int GetRenderTargetCount() const { return m_renderTargets.size(); }

    void InvalidateShader() { m_dirtyFlags |= DIRTY_SHADER; }
    void InvalidateTextures() { m_dirtyFlags |= DIRTY_TEXTURES; }
    void InvalidateBlend() { m_dirtyFlags |= DIRTY_BLEND; }
    void InvalidateDepth() { m_dirtyFlags |= DIRTY_DEPTH; }
    void InvalidateRasterizer() { m_dirtyFlags |= DIRTY_RASTERIZER; }
    void InvalidateSamplers() { m_dirtyFlags |= DIRTY_SAMPLERS; }
    void InvalidateViewport() { m_dirtyFlags |= DIRTY_VIEWPORT; }
    void InvalidateRenderTargets() { m_dirtyFlags |= DIRTY_RENDERTARGETS; }

    void InvalidateAll();

    void ApplyCurrentState(LPDIRECT3DDEVICE9 device);
    void ResetToDefaults();

private:
    enum DirtyFlags {
        DIRTY_NONE         = 0,
        DIRTY_SHADER       = 1 << 0,
        DIRTY_TEXTURES     = 1 << 1,
        DIRTY_BLEND        = 1 << 2,
        DIRTY_DEPTH        = 1 << 3,
        DIRTY_RASTERIZER   = 1 << 4,
        DIRTY_SAMPLERS     = 1 << 5,
        DIRTY_VIEWPORT     = 1 << 6,
        DIRTY_RENDERTARGETS = 1 << 7,
        DIRTY_ALL          = 0xFF
    };

    void ApplyShader(LPDIRECT3DDEVICE9 device);
    void ApplyTextures(LPDIRECT3DDEVICE9 device);
    void ApplyBlendState(LPDIRECT3DDEVICE9 device);
    void ApplyDepthState(LPDIRECT3DDEVICE9 device);
    void ApplyRasterizerState(LPDIRECT3DDEVICE9 device);
    void ApplySamplerStates(LPDIRECT3DDEVICE9 device);
    void ApplyViewport(LPDIRECT3DDEVICE9 device);
    void ApplyRenderTargets(LPDIRECT3DDEVICE9 device);

    static const int MAX_TEXTURE_SLOTS = 8;
    static const int MAX_RENDER_TARGETS = 4;

    LPDIRECT3DDEVICE9 m_device;

    int m_dirtyFlags;

    ShaderID m_currentShader;
    LPDIRECT3DBASETEXTURE9 m_currentTextures[MAX_TEXTURE_SLOTS];
    int m_currentTextureSlot;

    BlendState m_blendState;
    DepthState m_depthState;
    RasterizerState m_rasterizerState;
    SamplerState m_samplerStates[MAX_TEXTURE_SLOTS];
    Viewport m_viewport;

    std::vector<RenderTargetBinding> m_renderTargets;
};

}