#pragma once
#include <d3d9.h>
#include <vector>
#include "RenderTypes.h"

namespace Graphics {

enum RenderPassType {
    PASS_GEOMETRY,
    PASS_LIGHTING,
    PASS_TRANSPARENT,
    PASS_UI,
    PASS_POSTFX,
    PASS_COUNT
};

struct PassStats {
    int drawCalls;
    int batchCount;
    int triangles;
    float gpuTimeMs;
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
};

struct TransparentCommand {
    SpriteVertex vertices[4];
    void* texture;
    float depth;
    int shaderID;
    bool isUI;
};

struct UICommand {
    SpriteVertex vertices[4];
    void* texture;
    float screenX, screenY;
    float width, height;
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

    void BeginFrame();
    void EndFrame();

    void AddGeometryCommand(const GeometryCommand& cmd);
    void AddTransparentCommand(const TransparentCommand& cmd);
    void AddUICommand(const UICommand& cmd);
    void AddPostFXCommand(const PostFXCommand& cmd);

    void ExecuteGeometryPass();
    void ExecuteLightingPass();
    void ExecuteTransparentPass();
    void ExecuteUIPass();
    void ExecutePostFXPass();

    void Execute();

    void BindGBuffer();
    void UnbindGBuffer();
    void ClearGBuffers();
    void ApplyDeferredLighting(int debugView = 0);

    const PassStats& GetPassStats(RenderPassType pass) const;
    
    int GetTotalDrawCalls() const;
    int GetTotalBatches() const;

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

private:
    void ValidateRenderTargets();
    void ValidateShaders();
    void ValidateBlendState();
    void ValidateDepthState();

    LPDIRECT3DDEVICE9 m_pDevice;
    
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
};

}