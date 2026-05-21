#pragma once
#include <d3d9.h>
#include "GPUTimer.h"

namespace Graphics {

class RenderQueue;
class SpriteRenderer;
class TileRenderer;
class RenderDebugOverlay;

class RenderFrame {
public:
    RenderFrame();
    ~RenderFrame();

    void Initialize(LPDIRECT3DDEVICE9 pDevice);
    void Shutdown();

    void SetRenderQueue(RenderQueue* queue) { m_renderQueue = queue; }
    RenderQueue* GetRenderQueue() const { return m_renderQueue; }

    void SetSpriteRenderer(SpriteRenderer* renderer) { m_spriteRenderer = renderer; }
    SpriteRenderer* GetSpriteRenderer() const { return m_spriteRenderer; }

    void SetTileRenderer(TileRenderer* renderer) { m_tileRenderer = renderer; }
    TileRenderer* GetTileRenderer() const { return m_tileRenderer; }

    void SetGPUTimer(GPUTimer* timer) { m_gpuTimer = timer; }
    void SetDebugOverlay(RenderDebugOverlay* overlay) { m_debugOverlay = overlay; }
    RenderDebugOverlay* GetDebugOverlay() const { return m_debugOverlay; }

    void BeginFrame();
    void Execute();
    void EndFrame();

    IDirect3DSurface9* GetBackBuffer() const { return m_pBackBuffer; }
    bool IsInitialized() const { return m_initialized; }

private:
    LPDIRECT3DDEVICE9 m_pDevice;
    RenderQueue* m_renderQueue;
    SpriteRenderer* m_spriteRenderer;
    TileRenderer* m_tileRenderer;
    GPUTimer* m_gpuTimer;
    RenderDebugOverlay* m_debugOverlay;
    IDirect3DSurface9* m_pBackBuffer;
    bool m_initialized;
};

}
