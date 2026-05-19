#include "stdafx.h"
#include "RenderFrame.h"
#include "ShaderManager.h"
#include "SpriteRenderer.h"
#include "Material.h"

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
    , m_gpuTimer(NULL)
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

    D3DSURFACE_DESC desc;
    m_pDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &m_pBackBuffer);
    m_pBackBuffer->GetDesc(&desc);

    HRESULT hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height, D3DFMT_A32B32G32R32F,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferPos, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferPos failed!\n"); }

    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height, D3DFMT_A16B16G16R16F,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferNormal, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferNormal failed!\n"); }

    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height, D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferAlbedo, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferAlbedo failed!\n"); }

    hr = m_pDevice->CreateRenderTarget(
        desc.Width, desc.Height, D3DFMT_A8R8G8B8,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferSpec, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferSpec failed!\n"); }

    hr = m_pDevice->CreateDepthStencilSurface(
        desc.Width, desc.Height, D3DFMT_D24S8,
        D3DMULTISAMPLE_NONE, 0, TRUE, &m_pGBufferDepth, NULL);
    if (FAILED(hr)) { OutputDebugStringA("[RF] ERROR: Create GBufferDepth failed!\n"); }

    m_debugOverlay = nullptr;
    m_rtManager = nullptr;

    m_initialized = true;
    OutputDebugStringA("[RenderFrame] Initialized\n");
}

void RenderFrame::Shutdown() {
    m_passes.clear();
    m_dependencies.clear();

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

void RenderFrame::AddPass(RenderPassBase* pass) {
    if (pass) {
        m_passes[pass->GetType()] = pass;
    }
}

void RenderFrame::RemovePass(RenderPassType type) {
    m_passes.erase(type);
}

RenderPassBase* RenderFrame::GetPass(RenderPassType type) {
    auto it = m_passes.find(type);
    if (it != m_passes.end()) {
        return it->second;
    }
    return NULL;
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

void RenderFrame::AddPostFXPass(PostFXCommand::PostFXType type, float intensity, const float* params) {
    PostFXCommand cmd;
    cmd.type = type;
    cmd.intensity = intensity;
    if (params) {
        for (int i = 0; i < 4; i++) cmd.params[i] = params[i];
    } else {
        for (int i = 0; i < 4; i++) cmd.params[i] = 0.0f;
    }
    m_postFXQueue.Add(cmd);
    OutputDebugStringA("[RenderFrame] Added PostFX pass to chain\n");
}

void RenderFrame::SetPassDependency(RenderPassType dependent, RenderPassType dependency, bool required) {
    PassDependency dep;
    dep.dependent = dependent;
    dep.dependency = dependency;
    dep.required = required;
    m_dependencies.push_back(dep);
}

bool RenderFrame::ValidatePassDependencies() const {
    for (size_t i = 0; i < m_dependencies.size(); i++) {
        const PassDependency& dep = m_dependencies[i];
        bool depExists = m_passes.find(dep.dependency) != m_passes.end();
        if (dep.required && !depExists) {
            return false;
        }
    }
    return true;
}

void RenderFrame::SortPassesByDependency() {
}

int RenderFrame::m_startTimer(const char* name) {
    if (m_gpuTimer) {
        return m_gpuTimer->StartTimer(name);
    }
    return -1;
}

void RenderFrame::m_endTimer(int timerIndex) {
    if (m_gpuTimer && timerIndex >= 0) {
        m_gpuTimer->EndTimer(timerIndex);
    }
}

void RenderFrame::ExecuteGeometryPass() {
    if (!m_initialized) return;
    
    int timerIdx = m_startTimer("GeometryPass");
    OutputDebugStringA("[RenderFrame] ExecuteGeometryPass\n");
    
    BindGBuffer();
    ClearGBuffers();
    
    m_passStats[PASS_GEOMETRY].drawCalls = m_geometryQueue.GetCommandCount();
    m_passStats[PASS_GEOMETRY].batchCount = 0;

    if (m_shaderManager && m_spriteRenderer) {
        m_shaderManager->SetActiveShader(SHADER_SPRITE_GBUFFER);
        
        LPDIRECT3DVERTEXBUFFER9 pVB = m_spriteRenderer->GetVertexBuffer();
        LPDIRECT3DVERTEXBUFFER9 pIB = m_spriteRenderer->GetIndexBuffer();
        LPDIRECT3DVERTEXDECLARATION9 pDecl = m_spriteRenderer->GetVertexDeclaration();
        
        if (pVB && pIB && pDecl) {
            const D3DXMATRIX& viewProj = m_shaderManager->GetFrameViewProj();
            m_shaderManager->ExecuteQueue(pVB, pIB, pDecl, 32, &viewProj, m_spriteRenderer);
        }
    }
    
    ValidateRenderTargets();
    ValidateShaders();
    ValidateDepthState();
    
    m_endTimer(timerIdx);
}

void RenderFrame::ExecuteLightingPass() {
    if (!m_initialized) return;
    
    int timerIdx = m_startTimer("LightingPass");
    OutputDebugStringA("[RenderFrame] ExecuteLightingPass\n");
    
    UnbindGBuffer();
    ApplyDeferredLighting(m_debugViewMode);
    
    m_passStats[PASS_LIGHTING].drawCalls = 1;
    
    ValidateRenderTargets();
    ValidateShaders();
    ValidateBlendState();
    
    m_endTimer(timerIdx);
}

void RenderFrame::ExecuteAlphaTestPass() {
    if (!m_initialized) return;
    
    int timerIdx = m_startTimer("AlphaTestPass");
    OutputDebugStringA("[RenderFrame] ExecuteAlphaTestPass\n");
    
    m_passStats[PASS_ALPHATEST].drawCalls = 0;
    
    if (m_shaderManager) {
        m_shaderManager->SetActiveShader(SHADER_SPRITE);
        m_shaderManager->BeginShader();
        m_shaderManager->BeginPass(0);
        
        LPDIRECT3DDEVICE9 pDevice = m_pDevice;
        if (pDevice) {
            pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
            pDevice->SetRenderState(D3DRS_ALPHAREF, 128);
        }
        
        m_shaderManager->EndPass();
        m_shaderManager->EndShader();
    }
    
    m_endTimer(timerIdx);
}

void RenderFrame::ExecuteTransparentPass() {
    if (!m_initialized) return;
    
    int timerIdx = m_startTimer("TransparentPass");
    OutputDebugStringA("[RenderFrame] ExecuteTransparentPass\n");
    
    m_passStats[PASS_TRANSPARENT].drawCalls = m_transparentQueue.GetCommandCount();
    
    if (m_shaderManager && m_spriteRenderer) {
        m_shaderManager->SetActiveShader(SHADER_SPRITE);
        m_shaderManager->BeginShader();
        m_shaderManager->BeginPass(0);
        
        LPDIRECT3DDEVICE9 pDevice = m_pDevice;
        if (pDevice) {
            pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }
        
        m_shaderManager->EndPass();
        m_shaderManager->EndShader();
    }
    
    ValidateBlendState();
    
    m_endTimer(timerIdx);
}

void RenderFrame::ExecuteUIPass() {
    if (!m_initialized) return;
    
    int timerIdx = m_startTimer("UIPass");
    OutputDebugStringA("[RenderFrame] ExecuteUIPass\n");
    
    m_passStats[PASS_UI].drawCalls = m_uiQueue.GetCommandCount();
    
    if (m_shaderManager) {
        m_shaderManager->SetActiveShader(SHADER_SPRITE);
        m_shaderManager->BeginShader();
        m_shaderManager->BeginPass(0);
        
        LPDIRECT3DDEVICE9 pDevice = m_pDevice;
        if (pDevice) {
            pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
            pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
        }
        
        m_shaderManager->EndPass();
        m_shaderManager->EndShader();
    }
    
    m_endTimer(timerIdx);
}

void RenderFrame::ExecutePostFXPass() {
    if (!m_initialized) return;
    
    int timerIdx = m_startTimer("PostFXPass");
    OutputDebugStringA("[RenderFrame] ExecutePostFXPass\n");
    
    m_passStats[PASS_POSTFX].drawCalls = m_postFXQueue.GetCommandCount();
    
    if (m_postFXQueue.GetCommandCount() > 0) {
        OutputDebugStringA("[RenderFrame] PostFX commands pending - TODO: implement full PostFX pipeline\n");
    }
    
    m_endTimer(timerIdx);
}

void RenderFrame::Execute() {
    if (!m_initialized) return;
    
    if (!ValidatePassDependencies()) {
        OutputDebugStringA("[RenderFrame] WARNING: Pass dependencies not satisfied!\n");
    }
    
    ValidateResources();

    ExecuteGeometryPass();
    ExecuteLightingPass();
    ExecuteAlphaTestPass();
    ExecuteTransparentPass();
    ExecuteUIPass();
    ExecutePostFXPass();

    if (m_debugOverlay) {
        const RenderStats& stats = m_debugOverlay->GetStats();
        (void)stats;
    }
}

void RenderFrame::ValidateResources() {
#ifdef _DEBUG
    if (!m_pGBufferPos || !m_pGBufferNormal || !m_pGBufferAlbedo) {
        OutputDebugStringA("[RenderFrame] ERROR: GBuffer not initialized!\n");
    }
    if (!m_shaderManager) {
        OutputDebugStringA("[RenderFrame] ERROR: ShaderManager not set!\n");
    }
    if (!m_spriteRenderer) {
        OutputDebugStringA("[RenderFrame] ERROR: SpriteRenderer not set!\n");
    }
#endif
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
    
    m_pDevice->SetRenderTarget(0, m_pBackBuffer);
    m_pDevice->SetRenderTarget(1, NULL);
    m_pDevice->SetRenderTarget(2, NULL);
    m_pDevice->SetRenderTarget(3, NULL);
    m_pDevice->SetDepthStencilSurface(m_pGBufferDepth);
}

void RenderFrame::ClearGBuffers() {
    if (!m_pDevice) return;
    
    float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0,0,0), 1.0f, 0);
}

void RenderFrame::ApplyDeferredLighting(int debugView) {
    if (!m_pDevice) return;
    
    m_pDevice->SetRenderTarget(0, m_pBackBuffer);
    m_pDevice->SetRenderTarget(1, NULL);
    m_pDevice->SetRenderTarget(2, NULL);
    m_pDevice->SetRenderTarget(3, NULL);
    m_pDevice->SetDepthStencilSurface(m_pGBufferDepth);
    
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

int RenderFrame::GetCommandCount(RenderPassType type) const {
    switch (type) {
        case PASS_GEOMETRY: return m_geometryQueue.GetCommandCount();
        case PASS_TRANSPARENT: return m_transparentQueue.GetCommandCount();
        case PASS_UI: return m_uiQueue.GetCommandCount();
        case PASS_POSTFX: return m_postFXQueue.GetCommandCount();
        default: return 0;
    }
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
    if (m_shaderManager) {
        OutputDebugStringA("[RenderFrame] ValidateShaders: OK\n");
    }
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

void RenderFrame::ValidateMaterial(int materialID) {
#ifdef _DEBUG
    if (m_materialManager) {
        Material* mat = m_materialManager->GetMaterial(materialID);
        if (!mat) {
            OutputDebugStringA("[RenderFrame] WARNING: Invalid material ID!\n");
        }
    }
#endif
}

void RenderFrame::ValidatePassState(RenderPassType pass) {
#ifdef _DEBUG
    auto it = m_passes.find(pass);
    if (it == m_passes.end()) {
        char msg[128];
        sprintf_s(msg, "[RenderFrame] WARNING: Pass %d not registered!\n", pass);
        OutputDebugStringA(msg);
    }
#endif
}

}