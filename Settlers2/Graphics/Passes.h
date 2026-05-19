#pragma once
#include "RenderPassBase.h"
#include "RenderFrame.h"
#include "ShaderManager.h"
#include "SpriteRenderer.h"
#include "Material.h"
#include "GPUTimer.h"

namespace Graphics {

class GeometryPass : public RenderPassBase {
public:
    GeometryPass(ShaderManager* shaderMgr, SpriteRenderer* spriteRenderer, GPUTimer* timer)
        : RenderPassBase("GeometryPass", PASS_GEOMETRY, 0)
        , m_shaderManager(shaderMgr)
        , m_spriteRenderer(spriteRenderer)
        , m_gpuTimer(timer)
    {}

    void Execute() override;

    void SetGBufferSurfaces(LPDIRECT3DSURFACE9 pos, LPDIRECT3DSURFACE9 normal, 
                           LPDIRECT3DSURFACE9 albedo, LPDIRECT3DSURFACE9 spec,
                           LPDIRECT3DSURFACE9 depth) {
        m_gBufferPos = pos;
        m_gBufferNormal = normal;
        m_gBufferAlbedo = albedo;
        m_gBufferSpec = spec;
        m_gBufferDepth = depth;
    }

private:
    ShaderManager* m_shaderManager;
    SpriteRenderer* m_spriteRenderer;
    GPUTimer* m_gpuTimer;
    LPDIRECT3DSURFACE9 m_gBufferPos;
    LPDIRECT3DSURFACE9 m_gBufferNormal;
    LPDIRECT3DSURFACE9 m_gBufferAlbedo;
    LPDIRECT3DSURFACE9 m_gBufferSpec;
    LPDIRECT3DSURFACE9 m_gBufferDepth;
};

class LightingPass : public RenderPassBase {
public:
    LightingPass(ShaderManager* shaderMgr, GPUTimer* timer)
        : RenderPassBase("LightingPass", PASS_LIGHTING, 1)
        , m_shaderManager(shaderMgr)
        , m_gpuTimer(timer)
        , m_backBuffer(NULL)
        , m_gBufferDepth(NULL)
    {}

    void Execute() override;

    void SetBackBuffer(LPDIRECT3DSURFACE9 backBuffer) { m_backBuffer = backBuffer; }
    void SetGBufferDepth(LPDIRECT3DSURFACE9 depth) { m_gBufferDepth = depth; }

private:
    ShaderManager* m_shaderManager;
    GPUTimer* m_gpuTimer;
    LPDIRECT3DSURFACE9 m_backBuffer;
    LPDIRECT3DSURFACE9 m_gBufferDepth;
};

class AlphaTestPass : public RenderPassBase {
public:
    AlphaTestPass(ShaderManager* shaderMgr, GPUTimer* timer)
        : RenderPassBase("AlphaTestPass", PASS_ALPHATEST, 2)
        , m_shaderManager(shaderMgr)
        , m_gpuTimer(timer)
    {}

    void Execute() override;

private:
    ShaderManager* m_shaderManager;
    GPUTimer* m_gpuTimer;
};

class TransparentPass : public RenderPassBase {
public:
    TransparentPass(ShaderManager* shaderMgr, GPUTimer* timer)
        : RenderPassBase("TransparentPass", PASS_TRANSPARENT, 3)
        , m_shaderManager(shaderMgr)
        , m_gpuTimer(timer)
    {}

    void Execute() override;

private:
    ShaderManager* m_shaderManager;
    GPUTimer* m_gpuTimer;
};

class UIPass : public RenderPassBase {
public:
    UIPass(ShaderManager* shaderMgr, GPUTimer* timer)
        : RenderPassBase("UIPass", PASS_UI, 4)
        , m_shaderManager(shaderMgr)
        , m_gpuTimer(timer)
    {}

    void Execute() override;

private:
    ShaderManager* m_shaderManager;
    GPUTimer* m_gpuTimer;
};

class PostFXPass : public RenderPassBase {
public:
    PostFXPass(GPUTimer* timer)
        : RenderPassBase("PostFXPass", PASS_POSTFX, 5)
        , m_gpuTimer(timer)
    {}

    void Execute() override;
    void AddEffect(PostFXCommand::PostFXType type, float intensity, const float* params);

private:
    GPUTimer* m_gpuTimer;
    std::vector<PostFXCommand> m_effects;
};

}