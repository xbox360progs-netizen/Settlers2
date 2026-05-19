#include "stdafx.h"
#include "RenderDocTool.h"
#include "Material.h"

namespace Graphics {

static void OutputDebugStringA(const char* msg) {
#ifdef _DEBUG
    ::OutputDebugStringA(msg);
#endif
}

RenderDocIntegration::RenderDocIntegration()
    : m_pDevice(NULL)
    , m_available(false)
    , m_capturingFrame(false)
    , m_autoCapture(false)
    , m_captureAfterFrames(0)
    , m_frameCounter(0)
{
}

RenderDocIntegration::~RenderDocIntegration() {
    Shutdown();
}

void RenderDocIntegration::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_available = false;
    OutputDebugStringA("[RenderDocIntegration] Initialized (placeholder - requires RenderDoc SDK)\n");
}

void RenderDocIntegration::Shutdown() {
    m_pDevice = NULL;
}

void RenderDocIntegration::TriggerCapture() {
    OutputDebugStringA("[RenderDoc] Trigger capture\n");
}

void RenderDocIntegration::StartFrameCapture() {
    m_capturingFrame = true;
    OutputDebugStringA("[RenderDoc] Start frame capture\n");
}

void RenderDocIntegration::EndFrameCapture() {
    m_capturingFrame = false;
    OutputDebugStringA("[RenderDoc] End frame capture\n");
}

RenderDocManager::RenderDocManager()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_overlayEnabled(false)
    , m_currentView(VIEW_NONE)
{
}

RenderDocManager::~RenderDocManager() {
    Shutdown();
}

void RenderDocManager::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_integration.Initialize(pDevice);
    m_initialized = true;
    OutputDebugStringA("[RenderDocManager] Initialized\n");
}

void RenderDocManager::Shutdown() {
    m_integration.Shutdown();
    m_initialized = false;
}

void RenderDocManager::BeginFrame() {
    if (m_integration.GetAutoCapture()) {
        m_integration.BeginFrame();
    }
}

void RenderDocManager::EndFrame() {
    if (m_integration.IsFrameCapturing()) {
        m_integration.EndFrameCapture();
    }
}

void RenderDocManager::RenderUI() {
    if (!m_overlayEnabled || !m_initialized) return;

    char buf[256];
    int y = 50;

    sprintf(buf, "=== RENDER DOC TOOL ===");
    OutputDebugStringA(buf);

    sprintf(buf, "Current View: %s", GetViewName());
    OutputDebugStringA(buf);

    sprintf(buf, "View: [Left/Right] Cycle, [Tab] Toggle");
    OutputDebugStringA(buf);
}

void RenderDocManager::NextView() {
    m_currentView = (DebugView)((m_currentView + 1) % VIEW_COUNT);
}

void RenderDocManager::PreviousView() {
    m_currentView = (DebugView)((m_currentView - 1 + VIEW_COUNT) % VIEW_COUNT);
}

HotShaderReloader::HotShaderReloader()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_enabled(true)
    , m_reloadCount(0)
{
}

HotShaderReloader::~HotShaderReloader() {
    Shutdown();
}

void HotShaderReloader::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_initialized = true;
    OutputDebugStringA("[HotShaderReloader] Initialized\n");
}

void HotShaderReloader::Shutdown() {
    m_watchedShaders.clear();
    m_initialized = false;
}

void HotShaderReloader::BeginFrame() {
}

void HotShaderReloader::EndFrame() {
    if (!m_enabled) return;
    CheckForChanges();
}

void HotShaderReloader::WatchShader(const char* shaderPath) {
    ShaderFileInfo info;
    info.path = shaderPath;
    info.lastWriteTime = GetFileWriteTime(shaderPath);
    info.needsReload = false;
    m_watchedShaders.push_back(info);

    char buf[256];
    sprintf(buf, "[HotReload] Watching: %s\n", shaderPath);
    OutputDebugStringA(buf);
}

void HotShaderReloader::StopWatching(const char* shaderPath) {
    for (size_t i = 0; i < m_watchedShaders.size(); i++) {
        if (m_watchedShaders[i].path == shaderPath) {
            m_watchedShaders.erase(m_watchedShaders.begin() + i);
            return;
        }
    }
}

void HotShaderReloader::CheckForChanges() {
    for (size_t i = 0; i < m_watchedShaders.size(); i++) {
        long currentTime = GetFileWriteTime(m_watchedShaders[i].path.c_str());
        if (currentTime > m_watchedShaders[i].lastWriteTime) {
            m_watchedShaders[i].needsReload = true;
            m_watchedShaders[i].lastWriteTime = currentTime;
        }
    }
}

void HotShaderReloader::ReloadChangedShaders() {
    for (size_t i = 0; i < m_watchedShaders.size(); i++) {
        if (m_watchedShaders[i].needsReload) {
            OutputDebugStringA("[HotReload] Reloading shader\n");
            m_reloadCount++;
            m_watchedShaders[i].needsReload = false;
        }
    }
}

long HotShaderReloader::GetFileWriteTime(const char* path) {
    return 0;
}

LiveMaterialEditor::LiveMaterialEditor()
    : m_pDevice(NULL)
    , m_materialManager(NULL)
    , m_initialized(false)
    , m_enabled(false)
    , m_selectedMaterial(-1)
    , m_propertyEditorTarget(-1)
{
}

LiveMaterialEditor::~LiveMaterialEditor() {
    Shutdown();
}

void LiveMaterialEditor::Initialize(IDirect3DDevice9* pDevice, MaterialManager* materialManager) {
    m_pDevice = pDevice;
    m_materialManager = materialManager;
    m_initialized = true;
    OutputDebugStringA("[LiveMaterialEditor] Initialized\n");
}

void LiveMaterialEditor::Shutdown() {
    m_initialized = false;
}

void LiveMaterialEditor::BeginFrame() {
}

void LiveMaterialEditor::EndFrame() {
}

void LiveMaterialEditor::RenderUI() {
    if (!m_enabled || !m_initialized) return;

    OutputDebugStringA("[LiveMaterialEditor] Render UI\n");
}

void LiveMaterialEditor::SelectMaterial(int materialID) {
    m_selectedMaterial = materialID;
}

void LiveMaterialEditor::SetMaterialPropertyFloat(int materialID, const char* property, float value) {
    OutputDebugStringA("[LiveMaterialEditor] Set property\n");
}

void LiveMaterialEditor::SetMaterialPropertyColor(int materialID, const char* property, float r, float g, float b, float a) {
    OutputDebugStringA("[LiveMaterialEditor] Set color\n");
}

void LiveMaterialEditor::SetMaterialPropertyTexture(int materialID, const char* property, void* texture) {
    OutputDebugStringA("[LiveMaterialEditor] Set texture\n");
}

void LiveMaterialEditor::RenderMaterialList() {
}

void LiveMaterialEditor::RenderPropertyEditor() {
}

void LiveMaterialEditor::RenderTextureSlots() {
}

EngineToolingPanel::EngineToolingPanel()
    : m_pDevice(NULL)
    , m_initialized(false)
    , m_visible(false)
{
}

EngineToolingPanel::~EngineToolingPanel() {
    Shutdown();
}

void EngineToolingPanel::Initialize(IDirect3DDevice9* pDevice) {
    m_pDevice = pDevice;
    m_renderDocManager.Initialize(pDevice);
    m_shaderReloader.Initialize(pDevice);
    m_initialized = true;
    OutputDebugStringA("[EngineToolingPanel] Initialized\n");
}

void EngineToolingPanel::Shutdown() {
    m_renderDocManager.Shutdown();
    m_shaderReloader.Shutdown();
    m_initialized = false;
}

void EngineToolingPanel::BeginFrame() {
    if (!m_visible) return;
    m_renderDocManager.BeginFrame();
    m_shaderReloader.BeginFrame();
    m_materialEditor.BeginFrame();
}

void EngineToolingPanel::EndFrame() {
    if (!m_visible) return;
    m_renderDocManager.EndFrame();
    m_shaderReloader.EndFrame();
    m_materialEditor.EndFrame();
}

void EngineToolingPanel::Render() {
    if (!m_visible || !m_initialized) return;

    RenderTabs();
    RenderDebugSection();
    RenderPerformanceSection();
    RenderToolsSection();
}

void EngineToolingPanel::RenderTabs() {
}

void EngineToolingPanel::RenderDebugSection() {
    m_renderDocManager.RenderUI();
}

void EngineToolingPanel::RenderPerformanceSection() {
}

void EngineToolingPanel::RenderToolsSection() {
    m_materialEditor.RenderUI();
}

}