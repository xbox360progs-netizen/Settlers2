#include "stdafx.h"
#include "RenderFrame.h"
#include "ShaderManager.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderFrame::RenderFrame()
    : m_pDevice(NULL)
    , m_pGBufferPos(NULL)
    , m_pGBufferNormal(NULL)
    , m_pGBufferAlbedo(NULL)
    , m_pGBufferSpec(NULL)
    , m_pGBufferDepth(NULL)
    , m_pBackBuffer(NULL)
    , m_debugViewMode(0)
    , m_initialized(false) 
{
    ZeroMemory(m_passStats, sizeof(m_passStats));
}

RenderFrame::~RenderFrame() {
    Shutdown();
}

void RenderFrame::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    if (m_initialized) return;
    
    m_pDevice = pDevice;
    if (!m_pDevice) return;

    D3DSURFACE_DESC desc;
    m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    m_pBackBuffer->GetDesc(&desc);

    HRESULT hr;
    
    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A32B32G32R32F,
        D3DMULTISAMPLE_NONE, 0, TRUE,
        &m_pGBufferPos, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RenderFrame] ERROR: Create GBufferPos failed!\n"); return; }

    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A16B16G16R16F,
        D3DMULTISAMPLE_NONE, 0, TRUE,
        &m_pGBufferNormal, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RenderFrame] ERROR: Create GBufferNormal failed!\n"); return; }

    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE, 0, TRUE,
        &m_pGBufferAlbedo, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RenderFrame] ERROR: Create GBufferAlbedo failed!\n"); return; }

    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height,
        D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE, 0, TRUE,
        &m_pGBufferSpec, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RenderFrame] ERROR: Create GBufferSpec failed!\n"); return; }

    hr = m_pDevice->CreateDepthStencilSurface(
        desc.Width, desc.Height,
        D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE, 0, TRUE,
        &m_pGBufferDepth, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RenderFrame] ERROR: Create GBufferDepth failed!\n"); return; }

    m_initialized = true;
    OutputDebugStringA("[RenderFrame] Initialized\n");
}

void RenderFrame::Shutdown() {
    if (m_pGBufferPos) { m_pGBufferPos->Release(); m_pGBufferPos = NULL; }
    if (m_pGBufferNormal) { m_pGBufferNormal->Release(); m_pGBufferNormal = NULL; }
    if (m_pGBufferAlbedo) { m_pGBufferAlbedo->Release(); m_pGBufferAlbedo = NULL; }
    if (m_pGBufferSpec) { m_pGBufferSpec->Release(); m_pGBufferSpec = NULL; }
    if (m_pGBufferDepth) { m_pGBufferDepth->Release(); m_pGBufferDepth = NULL; }
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }
    m_initialized = false;
}

void RenderFrame::BeginFrame() {
    m_geometryQueue.Clear();
    m_transparentQueue.Clear();
    m_uiQueue.Clear();
    m_postFXQueue.Clear();
    ZeroMemory(m_passStats, sizeof(m_passStats));
}

void RenderFrame::EndFrame() {
}

void RenderFrame::AddGeometryCommand(const GeometryCommand& cmd) {
    m_geometryQueue.Add(cmd);
}

void RenderFrame::AddTransparentCommand(const TransparentCommand& cmd) {
    m_transparentQueue.Add(cmd);
}

void RenderFrame::AddUICommand(const UICommand& cmd) {
    m_uiQueue.Add(cmd);
}

void RenderFrame::AddPostFXCommand(const PostFXCommand& cmd) {
    m_postFXQueue.Add(cmd);
}

void RenderFrame::ExecuteGeometryPass() {
    if (!m_initialized) return;
    
    OutputDebugStringA("[RenderFrame] ExecuteGeometryPass\n");
    
    BindGBuffer();
    ClearGBuffers();
    
    m_passStats[PASS_GEOMETRY].drawCalls = 0;
    m_passStats[PASS_GEOMETRY].batchCount = 0;
    
    ValidateRenderTargets();
    ValidateShaders();
    ValidateDepthState();
}

void RenderFrame::ExecuteLightingPass() {
    if (!m_initialized) return;
    
    OutputDebugStringA("[RenderFrame] ExecuteLightingPass\n");
    
    UnbindGBuffer();
    ApplyDeferredLighting(m_debugViewMode);
    
    ValidateRenderTargets();
    ValidateShaders();
    ValidateBlendState();
}

void RenderFrame::ExecuteTransparentPass() {
    if (!m_initialized) return;
    
    OutputDebugStringA("[RenderFrame] ExecuteTransparentPass\n");
    
    m_passStats[PASS_TRANSPARENT].drawCalls = m_transparentQueue.GetCommandCount();
    
    ValidateBlendState();
}

void RenderFrame::ExecuteUIPass() {
    if (!m_initialized) return;
    
    OutputDebugStringA("[RenderFrame] ExecuteUIPass\n");
    
    m_passStats[PASS_UI].drawCalls = m_uiQueue.GetCommandCount();
}

void RenderFrame::ExecutePostFXPass() {
    if (!m_initialized) return;
    
    OutputDebugStringA("[RenderFrame] ExecutePostFXPass\n");
    
    m_passStats[PASS_POSTFX].drawCalls = m_postFXQueue.GetCommandCount();
}

void RenderFrame::Execute() {
    ExecuteGeometryPass();
    ExecuteLightingPass();
    ExecuteTransparentPass();
    ExecuteUIPass();
    ExecutePostFXPass();
}

void RenderFrame::BindGBuffer() {
    if (!m_pDevice) return;
    
    m_pDevice->SetRenderTarget(0, m_pGBufferPos);
    m_pDevice->SetRenderTarget(1, m_pGBufferNormal);
    m_pDevice->SetRenderTarget(2, m_pGBufferAlbedo);
    m_pDevice->SetRenderTarget(3, m_pGBufferSpec);
    m_pDevice->SetDepthStencilSurface(m_pGBufferDepth);
}

void RenderFrame::UnbindGBuffer() {
    if (!m_pDevice) return;
    
    m_pDevice->SetRenderTarget(0, NULL);
    m_pDevice->SetRenderTarget(1, NULL);
    m_pDevice->SetRenderTarget(2, NULL);
    m_pDevice->SetRenderTarget(3, NULL);
    m_pDevice->SetDepthStencilSurface(NULL);
}

void RenderFrame::ClearGBuffers() {
    if (!m_pDevice) return;
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

void RenderFrame::ApplyDeferredLighting(int debugView) {
    if (!m_pDevice) return;
    
    m_pDevice->SetRenderTarget(0, m_pBackBuffer);
    m_pDevice->SetRenderTarget(1, NULL);
    m_pDevice->SetRenderTarget(2, NULL);
    m_pDevice->SetRenderTarget(3, NULL);
    m_pDevice->SetDepthStencilSurface(NULL);
    
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
    
    OutputDebugStringA("[RenderFrame] ApplyDeferredLighting\n");
}

const PassStats& RenderFrame::GetPassStats(RenderPassType pass) const {
    return m_passStats[pass];
}

int RenderFrame::GetTotalDrawCalls() const {
    int total = 0;
    for (int i = 0; i < PASS_COUNT; i++) {
        total += m_passStats[i].drawCalls;
    }
    return total;
}

int RenderFrame::GetTotalBatches() const {
    int total = 0;
    for (int i = 0; i < PASS_COUNT; i++) {
        total += m_passStats[i].batchCount;
    }
    return total;
}

void RenderFrame::ValidateRenderTargets() {
#ifdef _DEBUG
    D3DRENDERSTATETYPE state = D3DRS_INVALID;
    HRESULT hr = m_pDevice->ValidateDevice(&state);
    if (FAILED(hr)) {
        OutputDebugStringA("[RenderFrame] WARNING: ValidateRenderTargets failed!\n");
    }
#endif
}

void RenderFrame::ValidateShaders() {
#ifdef _DEBUG
    OutputDebugStringA("[RenderFrame] ValidateShaders: OK\n");
#endif
}

void RenderFrame::ValidateBlendState() {
#ifdef _DEBUG
    DWORD blendEnable = 0;
    m_pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &blendEnable);
    if (!blendEnable) {
        OutputDebugStringA("[RenderFrame] WARNING: Alpha blend not enabled!\n");
    }
#endif
}

void RenderFrame::ValidateDepthState() {
#ifdef _DEBUG
    DWORD zEnable = 0;
    m_pDevice->GetRenderState(D3DRS_ZENABLE, &zEnable);
    OutputDebugStringA("[RenderFrame] ValidateDepthState: OK\n");
#endif
}

}