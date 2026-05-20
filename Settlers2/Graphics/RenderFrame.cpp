#include "stdafx.h"
#include "RenderFrame.h"
#include "ShaderManager.h"
#include "SpriteRenderer.h"
#include "Material.h"
#include "RenderContext.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderFrame::RenderFrame()
    : m_pDevice(NULL)
    , m_shaderManager(NULL)
    , m_spriteRenderer(NULL)
    , m_materialManager(NULL)
    , m_renderContext(NULL)
    , m_gpuTimer(NULL)
    , m_rtManager(NULL)
    , m_debugOverlay(NULL)
    , m_pBackBuffer(NULL)
    , m_debugViewMode(0)
    , m_initialized(false)
{
}

RenderFrame::~RenderFrame() {
    Shutdown();
}

void RenderFrame::Initialize(LPDIRECT3DDEVICE9 pDevice) {
    if (m_initialized) return;

    m_pDevice = pDevice;

    D3DSURFACE_DESC desc;
    pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    m_pBackBuffer->GetDesc(&desc);

    InitializeGBuffer(desc.Width, desc.Height);

    m_initialized = true;
    OutputDebugStringA("[RenderFrame] Initialized\n");
}

void RenderFrame::InitializeGBuffer(int width, int height) {
    HRESULT hr = S_OK;

    hr = m_pDevice->CreateRenderTarget(
        width, height, D3DFMT_A32B32G32R32F,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferPos, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferPos failed!\n"); }

    hr = m_pDevice->CreateRenderTarget(
        width, height, D3DFMT_A16B16G16R16F,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferNormal, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferNormal failed!\n"); }

    hr = m_pDevice->CreateRenderTarget(
        width, height, D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferAlbedo, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferAlbedo failed!\n"); }

    hr = m_pDevice->CreateRenderTarget(
        width, height, D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferSpec, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferSpec failed!\n"); }

    hr = m_pDevice->CreateDepthStencilSurface(
        width, height, D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferDepth, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferDepth failed!\n"); }
}

void RenderFrame::Shutdown() {
    DestroyPasses();

    if (m_pGBufferPos) { m_pGBufferPos->Release(); m_pGBufferPos = NULL; }
    if (m_pGBufferNormal) { m_pGBufferNormal->Release(); m_pGBufferNormal = NULL; }
    if (m_pGBufferAlbedo) { m_pGBufferAlbedo->Release(); m_pGBufferAlbedo = NULL; }
    if (m_pGBufferSpec) { m_pGBufferSpec->Release(); m_pGBufferSpec = NULL; }
    if (m_pGBufferDepth) { m_pGBufferDepth->Release(); m_pGBufferDepth = NULL; }
    if (m_pBackBuffer) { m_pBackBuffer->Release(); m_pBackBuffer = NULL; }

    m_initialized = false;
}

void RenderFrame::SetDependencies(ShaderManager* shaderMgr, ::SpriteRenderer* spriteRenderer, MaterialManager* materialMgr) {
    m_shaderManager = shaderMgr;
    m_spriteRenderer = spriteRenderer;
    m_materialManager = materialMgr;
}

void RenderFrame::CreateDefaultPasses() {
    if (!m_shaderManager || !m_spriteRenderer) return;

    GeometryPass* geometryPass = new GeometryPass(m_shaderManager, m_spriteRenderer, m_gpuTimer);
    geometryPass->SetRenderFrame(this);
    m_passes[PASS_GEOMETRY] = geometryPass;

    LightingPass* lightingPass = new LightingPass(m_shaderManager, m_gpuTimer);
    lightingPass->SetRenderFrame(this);
    m_passes[PASS_LIGHTING] = lightingPass;

    TransparentPass* transparentPass = new TransparentPass(m_shaderManager, m_gpuTimer);
    transparentPass->SetRenderFrame(this);
    m_passes[PASS_TRANSPARENT] = transparentPass;

    UIPass* uiPass = new UIPass(m_shaderManager, m_gpuTimer);
    uiPass->SetRenderFrame(this);
    m_passes[PASS_UI] = uiPass;

    OutputDebugStringA("[RenderFrame] Default passes created\n");
}

void RenderFrame::DestroyPasses() {
    for (auto& pair : m_passes) {
        delete pair.second;
    }
    m_passes.clear();
}

void RenderFrame::SetRenderQueue(RenderQueue* queue) {
    m_renderQueue = queue;
}

void RenderFrame::AddPass(IRenderPass* pass) {
    if (pass) {
        pass->SetContext(m_renderContext);
        m_passes[pass->GetType()] = pass;
    }
}

void RenderFrame::RemovePass(RenderPassType type) {
    m_passes.erase(type);
}

IRenderPass* RenderFrame::GetPass(RenderPassType type) {
    auto it = m_passes.find(type);
    if (it != m_passes.end()) {
        return it->second;
    }
    return NULL;
}

void RenderFrame::BeginFrame() {
    if (m_renderContext) {
        m_renderContext->BeginFrame();
    }
}

void RenderFrame::EndFrame() {
    if (m_renderContext) {
        m_renderContext->EndFrame();
    }
}

void RenderFrame::Execute() {
    if (!m_initialized) return;

    BeginFrame();

    if (m_gpuTimer) {
        m_gpuTimer->BeginFrame();
    }

    if (m_renderQueue) {
        m_renderQueue->Sort(SORT_BY_DEPTH_THEN_SHADER_THEN_TEXTURE);
        m_renderQueue->Batch();
    }

    IRenderPass* geometryPass = GetPass(PASS_GEOMETRY);
    if (geometryPass) {
        geometryPass->BeginPass();
        geometryPass->Execute();
        geometryPass->EndPass();
    }

    IRenderPass* lightingPass = GetPass(PASS_LIGHTING);
    if (lightingPass) {
        lightingPass->BeginPass();
        lightingPass->Execute();
        lightingPass->EndPass();
    }

    IRenderPass* transparentPass = GetPass(PASS_TRANSPARENT);
    if (transparentPass) {
        transparentPass->BeginPass();
        transparentPass->Execute();
        transparentPass->EndPass();
    }

    IRenderPass* uiPass = GetPass(PASS_UI);
    if (uiPass) {
        uiPass->BeginPass();
        uiPass->Execute();
        uiPass->EndPass();
    }

    if (m_gpuTimer) {
        m_gpuTimer->EndFrame();
    }

    if (m_debugOverlay) {
        m_debugOverlay->Render();
    }

    EndFrame();
}

}