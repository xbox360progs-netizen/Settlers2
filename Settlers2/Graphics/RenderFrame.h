#pragma once
#include <d3d9.h>
#include <vector>
#include <map>
#include "RenderTypes.h"
#include "GPUTimer.h"
#include "Material.h"

namespace Graphics {

class RenderPassBase;
class GPUTimer;
class RenderTargetManager;
class RenderDebugOverlay;

using ::ShaderManager;

enum RenderPassType {
    PASS_GEOMETRY,
    PASS_LIGHTING,
    PASS_ALPHATEST,
    PASS_TRANSPARENT,
    PASS_UI,
    PASS_POSTFX,
    PASS_RESOLVE,
    PASS_COUNT
};

struct PassStats {
    int drawCalls;
    int batchCount;
    int triangles;
    float gpuTimeMs;
};

enum RenderLayerType {
    LAYER_OPAQUE,
    LAYER_ALPHATEST,
    LAYER_TRANSPARENT,
    LAYER_UI
};

struct PassDependency {
    RenderPassType dependent;
    RenderPassType dependency;
    bool required;
};

class RenderQueueBase {
public:
    virtual ~RenderQueueBase() {}
    virtual void Clear() = 0;
    virtual void Sort() = 0;
    virtual int GetCommandCount() const = 0;
};

template<typename T>
class TypedRenderQueue : public RenderQueueBase {
public:
    void Add(const T& cmd) { m_commands.push_back(cmd); }
    const T* GetCommands() const { return m_commands.data(); }
    
    void Clear() override { m_commands.clear(); }
    void Sort() override {}
    int GetCommandCount() const override { return (int)m_commands.size(); }
    
    std::vector<T>& GetMutable() { return m_commands; }
    
private:
    std::vector<T> m_commands;
};

struct GeometryCommand {
    SpriteVertex vertices[4];
    int materialID;
    float depth;
    int shaderID;
    void* texture;
    RenderLayerType layer;
};

struct TransparentCommand {
    SpriteVertex vertices[4];
    void* texture;
    float depth;
    int shaderID;
    bool isUI;
    int materialID;
};

struct UICommand {
    SpriteVertex vertices[4];
    void* texture;
    float screenX, screenY;
    float width, height;
    int materialID;
};

struct PostFXCommand {
    enum PostFXType {
        POSTFX_BLOOM,
        POSTFX_SSAO,
        POSTFX_FOG,
        POSTFX_TONEMAP,
        POSTFX_COLORGRADE
    };
    PostFXType type;
    float intensity;
    float params[4];
};

class RenderFrame {
public:
    RenderFrame();
    ~RenderFrame();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void SetDependencies(ShaderManager* shaderMgr, ::SpriteRenderer* spriteRenderer, MaterialManager* materialMgr);
    void SetGPUTimer(GPUTimer* timer) { m_gpuTimer = timer; }
    void SetRenderTargetManager(RenderTargetManager* mgr) { m_rtManager = mgr; }
    void SetDebugOverlay(RenderDebugOverlay* overlay) { m_debugOverlay = overlay; }
    RenderDebugOverlay* GetDebugOverlay() const { return m_debugOverlay; }

    void BeginFrame();
    void EndFrame();

    void AddGeometryCommand(const GeometryCommand& cmd);
    void AddTransparentCommand(const TransparentCommand& cmd);
    void AddUICommand(const UICommand& cmd);
    void AddPostFXCommand(const PostFXCommand& cmd);
    void AddPostFXPass(PostFXCommand::PostFXType type, float intensity = 1.0f, const float* params = nullptr);

    void AddPass(RenderPassBase* pass);
    void RemovePass(RenderPassType type);
    RenderPassBase* GetPass(RenderPassType type);

    void Execute();
    void ExecuteGeometryPass();
    void ExecuteLightingPass();
    void ExecuteAlphaTestPass();
    void ExecuteTransparentPass();
    void ExecuteUIPass();
    void ExecutePostFXPass();

    void ValidateResources();

    void BindGBuffer();
    void UnbindGBuffer();
    void ClearGBuffers();
    void ApplyDeferredLighting(int debugView = 0);

    const PassStats& GetPassStats(RenderPassType pass) const;
    
    int GetTotalDrawCalls() const;
    int GetTotalBatches() const;
    int GetCommandCount(RenderPassType type) const;

    void SetDebugViewMode(int mode) { m_debugViewMode = mode; }
    int GetDebugViewMode() const { return m_debugViewMode; }

    void ToggleDebugView() {
        m_debugViewMode = (m_debugViewMode + 1) % 6;
    }

    static const int DEBUG_NONE = 0;
    static const int DEBUG_ALBEDO = 1;
    static const int DEBUG_NORMAL = 2;
    static const int DEBUG_DEPTH = 3;
    static const int DEBUG_SPECULAR = 4;
    static const int DEBUG_LIGHTING = 5;

    void SetPassDependency(RenderPassType dependent, RenderPassType dependency, bool required = true);
    bool ValidatePassDependencies() const;
    void SortPassesByDependency();

    bool IsInitialized() const { return m_initialized; }

private:
    void ValidateRenderTargets();
    void ValidateShaders();
    void ValidateBlendState();
    void ValidateDepthState();
    void ValidateMaterial(int materialID);
    void ValidatePassState(RenderPassType pass);

    int m_startTimer(const char* name);
    void m_endTimer(int timerIndex);

    LPDIRECT3DDEVICE9 m_pDevice;
    ShaderManager* m_shaderManager;
    ::SpriteRenderer* m_spriteRenderer;
    MaterialManager* m_materialManager;
    GPUTimer* m_gpuTimer;
    RenderTargetManager* m_rtManager;
    RenderDebugOverlay* m_debugOverlay;
    
    TypedRenderQueue<GeometryCommand> m_geometryQueue;
    TypedRenderQueue<TransparentCommand> m_transparentQueue;
    TypedRenderQueue<UICommand> m_uiQueue;
    TypedRenderQueue<PostFXCommand> m_postFXQueue;
    
    PassStats m_passStats[PASS_COUNT];
    
    LPDIRECT3DSURFACE9 m_pGBufferPos;
    LPDIRECT3DSURFACE9 m_pGBufferNormal;
    LPDIRECT3DSURFACE9 m_pGBufferAlbedo;
    LPDIRECT3DSURFACE9 m_pGBufferSpec;
    LPDIRECT3DSURFACE9 m_pGBufferDepth;
    LPDIRECT3DSURFACE9 m_pBackBuffer;
    
    int m_debugViewMode;
    
    bool m_initialized;

    std::map<RenderPassType, RenderPassBase*> m_passes;
    std::vector<PassDependency> m_dependencies;
};

}