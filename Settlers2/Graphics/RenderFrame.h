#pragma once
#include <d3d9.h>
#include <vector>
#include <map>
#include "RenderTypes.h"
#include "GPUTimer.h"

namespace Graphics {

class GPUTimer;
class RenderTargetManager;
class RenderDebugOverlay;
class ShaderManager;
class SpriteRenderer;

using ::ShaderManager;

class RenderFrame {
public:
    RenderFrame();
    ~RenderFrame();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void SetDependencies(ShaderManager* shaderMgr, ::SpriteRenderer* spriteRenderer);

    void SetGPUTimer(GPUTimer* timer) { m_gpuTimer = timer; }
    void SetRenderTargetManager(RenderTargetManager* mgr) { m_rtManager = mgr; }
    void SetDebugOverlay(RenderDebugOverlay* overlay) { m_debugOverlay = overlay; }
    RenderDebugOverlay* GetDebugOverlay() const { return m_debugOverlay; }

    void SetRenderQueue(RenderQueue* queue);
    RenderQueue* GetRenderQueue() const { return m_renderQueue; }

    void BeginFrame();
    void EndFrame();
    void Execute();

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

    IRenderPass* GetPass(RenderPassType type);
    void AddPass(IRenderPass* pass);
    void RemovePass(RenderPassType type);

    IDirect3DSurface9* GetGBufferPos() const { return m_pGBufferPos; }
    IDirect3DSurface9* GetGBufferNormal() const { return m_pGBufferNormal; }
    IDirect3DSurface9* GetGBufferAlbedo() const { return m_pGBufferAlbedo; }
    IDirect3DSurface9* GetGBufferSpec() const { return m_pGBufferSpec; }
    IDirect3DSurface9* GetGBufferDepth() const { return m_pGBufferDepth; }
    IDirect3DSurface9* GetBackBuffer() const { return m_pBackBuffer; }

    bool IsInitialized() const { return m_initialized; }

private:
    void InitializeGBuffer(int width, int height);

    LPDIRECT3DDEVICE9 m_pDevice;
    ShaderManager* m_shaderManager;
    ::SpriteRenderer* m_spriteRenderer;
    RenderQueue* m_renderQueue;
    GPUTimer* m_gpuTimer;
    RenderTargetManager* m_rtManager;
    RenderDebugOverlay* m_debugOverlay;

    IDirect3DSurface9 m_pGBufferPos;
    IDirect3DSurface9 m_pGBufferNormal;
    IDirect3DSurface9 m_pGBufferAlbedo;
    IDirect3DSurface9 m_pGBufferSpec;
    IDirect3DSurface9 m_pGBufferDepth;
    IDirect3DSurface9 m_pBackBuffer;

    int m_debugViewMode;
    bool m_initialized;
};

}