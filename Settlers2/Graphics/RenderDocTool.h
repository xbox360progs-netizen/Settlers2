#pragma once
#include <d3d9.h>
#include <string>
#include <vector>

namespace Graphics {

struct RenderDocFeature {
    const char* name;
    bool enabled;
    const char* description;
};

class RenderDocIntegration {
public:
    RenderDocIntegration();
    ~RenderDocIntegration();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    bool IsAvailable() const { return m_available; }
    bool IsFrameCapturing() const { return m_capturingFrame; }

    void TriggerCapture();
    void StartFrameCapture();
    void EndFrameCapture();

    void SetAutoCapture(bool autoCapture) { m_autoCapture = autoCapture; }
    bool GetAutoCapture() const { return m_autoCapture; }

    void SetCaptureAfterFrames(int frames) { m_captureAfterFrames = frames; }
    int GetCaptureAfterFrames() const { return m_captureAfterFrames; }

private:
    IDirect3DDevice9* m_pDevice;
    bool m_available;
    bool m_capturingFrame;
    bool m_autoCapture;
    int m_captureAfterFrames;
    int m_frameCounter;
};

class RenderDocManager {
public:
    RenderDocManager();
    ~RenderDocManager();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void RenderUI();

    void ToggleOverlay() { m_overlayEnabled = !m_overlayEnabled; }
    void NextView();
    void PreviousView();

    enum DebugView {
        VIEW_NONE,
        VIEW_ALBEDO,
        VIEW_NORMAL,
        VIEW_DEPTH,
        VIEW_SPECULAR,
        VIEW_LIGHTING,
        VIEW_MRT0,
        VIEW_MRT1,
        VIEW_MRT2,
        VIEW_MRT3,
        VIEW_SHADOWS,
        VIEW_AO,
        VIEW_OVERDRAW,
        VIEW_COUNT
    };

    void SetView(DebugView view) { m_currentView = view; }
    DebugView GetView() const { return m_currentView; }

    const char* GetViewName() const {
        static const char* names[] = {
            "None", "Albedo", "Normal", "Depth", "Specular", "Lighting",
            "MRT0", "MRT1", "MRT2", "MRT3", "Shadows", "AO", "Overdraw"
        };
        return names[m_currentView];
    }

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    bool m_overlayEnabled;
    DebugView m_currentView;
    RenderDocIntegration m_integration;
};

class HotShaderReloader {
public:
    HotShaderReloader();
    ~HotShaderReloader();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void WatchShader(const char* shaderPath);
    void StopWatching(const char* shaderPath);

    void CheckForChanges();
    void ReloadChangedShaders();

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    int GetReloadCount() const { return m_reloadCount; }

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    bool m_enabled;
    int m_reloadCount;

    struct ShaderFileInfo {
        std::string path;
        long lastWriteTime;
        bool needsReload;
    };

    std::vector<ShaderFileInfo> m_watchedShaders;

    long GetFileWriteTime(const char* path);
};

class LiveMaterialEditor {
public:
    LiveMaterialEditor();
    ~LiveMaterialEditor();

    void Initialize(IDirect3DDevice9* pDevice, class MaterialManager* materialManager);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void RenderUI();

    void SelectMaterial(int materialID);
    void SetMaterialPropertyFloat(int materialID, const char* property, float value);
    void SetMaterialPropertyColor(int materialID, const char* property, float r, float g, float b, float a);
    void SetMaterialPropertyTexture(int materialID, const char* property, void* texture);

    void SetEnabled(bool enabled) { m_enabled = enabled; }
    bool IsEnabled() const { return m_enabled; }

    void Toggle() { m_enabled = !m_enabled; }

private:
    IDirect3DDevice9* m_pDevice;
    class MaterialManager* m_materialManager;
    bool m_initialized;
    bool m_enabled;
    int m_selectedMaterial;
    int m_propertyEditorTarget;

    void RenderMaterialList();
    void RenderPropertyEditor();
    void RenderTextureSlots();
};

class EngineToolingPanel {
public:
    EngineToolingPanel();
    ~EngineToolingPanel();

    void Initialize(IDirect3DDevice9* pDevice);
    void Shutdown();

    void BeginFrame();
    void EndFrame();
    void Render();

    void TogglePanel() { m_visible = !m_visible; }
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }

    RenderDocManager* GetRenderDocManager() { return &m_renderDocManager; }
    HotShaderReloader* GetShaderReloader() { return &m_shaderReloader; }
    LiveMaterialEditor* GetMaterialEditor() { return &m_materialEditor; }

private:
    IDirect3DDevice9* m_pDevice;
    bool m_initialized;
    bool m_visible;

    RenderDocManager m_renderDocManager;
    HotShaderReloader m_shaderReloader;
    LiveMaterialEditor m_materialEditor;

    void RenderTabs();
    void RenderDebugSection();
    void RenderPerformanceSection();
    void RenderToolsSection();
};

}