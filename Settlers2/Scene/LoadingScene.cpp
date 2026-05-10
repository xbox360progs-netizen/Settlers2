#include "stdafx.h"
#include "LoadingScene.h"
#include "../Graphics/TextureRegistry.h"
// Note: LoadingScene centralizes texture loading via TextureRegistry
#include "../Graphics/Texture.h"
#include <functional>
#include <cstdio>
#include <iostream>

namespace Scene {

LoadingScene::LoadingScene()
    : Scene("Loading")
    , m_currentTaskIndex(0)
    , m_completedWeight(0.0f)
    , m_totalWeight(0.0f)
    , m_currentTaskProgress(0.0f)
    , m_loadProgress(0.0f)
    , m_loadingComplete(false)
    , m_textureLoader(nullptr)
    , m_renderer(nullptr)
    , m_spriteRenderer(nullptr)
    , m_binFileManager(nullptr)
    , m_screenW(1280.0f)
    , m_screenH(720.0f)
{
}

LoadingScene::~LoadingScene()
{
}

void LoadingScene::SetTargetScene(const std::string& sceneName)
{
    m_targetScene = sceneName;
}

void LoadingScene::Load()
{
    std::cout << "[LoadingScene] Load() called" << std::endl;
    std::cout << "[LoadingScene] m_textureLoader = " << (m_textureLoader ? "VALID" : "NULL") << std::endl;
    std::cout << "[LoadingScene] m_renderer = " << (m_renderer ? "VALID" : "NULL") << std::endl;
    std::cout << "[LoadingScene] m_spriteRenderer = " << (m_spriteRenderer ? "VALID" : "NULL") << std::endl;
    std::cout << "[LoadingScene] m_binFileManager = " << (m_binFileManager ? "VALID" : "NULL") << std::endl;
    
    m_currentTaskIndex = 0;
    m_completedWeight = 0.0f;
    m_totalWeight = 0.0f;
    m_currentTaskProgress = 0.0f;
    m_loadProgress = 0.0f;
    m_loadingComplete = false;
    m_statusText = "Loading...";
    
    // Initialize texture registry device bindings and manifest-based paths
    if (m_renderer) {
        TextureRegistry::instance().initialize(m_renderer->GetDevice());
        // Set BinFileManager for atlas loading support using static helper
        extern void SetBinFileManagerStatic(BinFileManager* mgr);
        if (m_binFileManager) {
            SetBinFileManagerStatic(m_binFileManager);
        }
        // Load manifest-based texture paths
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "");
    }
    
    // Clear previous tasks
    m_loadTasks.clear();
    
    // Load background texture first using TextureRegistry
    std::cout << "[LoadingScene] Loading background texture via TextureRegistry..." << std::endl;
    LPDIRECT3DTEXTURE9 pBgTex = TextureRegistry::instance().getTextureOrLoad("loading_background");
    if (pBgTex) {
        m_backgroundTexture.SetTexture(pBgTex);
        std::cout << "[LoadingScene] Background texture loaded successfully via TextureRegistry" << std::endl;
    } else {
        std::cout << "[LoadingScene] WARNING: Failed to load background texture via TextureRegistry" << std::endl;
        // Fallback to direct loading
        if (m_textureLoader && m_renderer) {
            HRESULT hr = m_textureLoader->Load(L"game:\\Media\\Textures\\Background\\loading_background.png", &pBgTex);
            if (SUCCEEDED(hr) && pBgTex) {
                m_backgroundTexture.SetTexture(pBgTex);
                TextureRegistry::instance().registerTexture("loading_background", pBgTex);
                std::cout << "[LoadingScene] Background texture loaded via fallback method" << std::endl;
            } else {
                std::cout << "[LoadingScene] ERROR: Failed to load background texture via fallback" << std::endl;
            }
        }
    }
    
    // Setup all load tasks using TextureRegistry approach
    std::cout << "[LoadingScene] Setting up load tasks..." << std::endl;
    SetupLoadTasks();
    std::cout << "[LoadingScene] Load tasks setup complete, total tasks: " << m_loadTasks.size() << std::endl;
    
    // Diagnostics
    TextureRegistry::instance().logManifestPathsStatus();

    m_loaded = true;
    std::cout << "[LoadingScene] Load() complete" << std::endl;
}

void LoadingScene::Unload()
{
    m_loadTasks.clear();
    m_loaded = false;
}

void LoadingScene::LoadAtlasOrTexture(const char* name, const char* pngPath)
{
    if (!m_binFileManager || !m_renderer) {
        return;
    }

    char logInit[512];
    _snprintf(logInit, sizeof(logInit), "[LoadingScene] LoadAtlasOrTexture: %s <- %s\n", name, pngPath);
    OutputDebugStringA(logInit);

    // ���������, �������� �� ��� ����� �����
    if (m_binFileManager->HasAtlas(name)) {
        _snprintf(logInit, sizeof(logInit), "[LoadingScene] Atlas %s already loaded\n", name);
        OutputDebugStringA(logInit);
        return;
    }

    // Construct BIN file path from PNG path
    std::string pngPathStr(pngPath);
    size_t dotPos = pngPathStr.rfind('.');
    if (dotPos == std::string::npos) {
        return;
    }
    std::string binPath = pngPathStr.substr(0, dotPos) + ".bin";

    // Check if BIN file exists
    std::wstring binPathW(binPath.begin(), binPath.end());
    FILE* binFile = _wfopen(binPathW.c_str(), L"rb");
    bool binExists = (binFile != NULL);
    if (binFile) {
        fclose(binFile);
    }

    char debugMsg[512];
    sprintf(debugMsg, "[LoadingScene] LoadAtlasOrTexture: name=%s, png=%s, bin=%s, binExists=%d\n", 
            name, pngPath, binPath.c_str(), binExists);
    OutputDebugStringA(debugMsg);

    if (binExists) {
        // Load from BIN file and register in TextureRegistry
        std::tr1::shared_ptr<SpriteAtlas> atlas = m_binFileManager->LoadAtlas(binPath, name);
        if (atlas) {
            TextureRegistry::instance().registerAtlas(name, atlas);
        }
    } else {
        // Load as single texture
        std::string textureName = name;
        LPDIRECT3DTEXTURE9 tex = TextureRegistry::instance().getTexture(textureName);
        if (tex) {
            _snprintf(debugMsg, sizeof(debugMsg), "[LoadingScene] Using existing texture for atlas: %s\n", name);
            OutputDebugStringA(debugMsg);
        }
        // Create atlas from single texture and register in TextureRegistry
        std::tr1::shared_ptr<SpriteAtlas> atlas = m_binFileManager->CreateAtlasFromSingleTexture(m_renderer->GetDevice(), name, pngPath);
        if (atlas) {
            TextureRegistry::instance().registerAtlas(name, atlas);
        }
    }
}

void LoadingScene::SetupLoadTasks()
{
    // ===== BACKGROUND TEXTURES =====
    AddLoadTask([this]() {
        // ��� ��������� ����� TextureRegistry � Load()
    }, "Load Background Textures", 0.5f);

    // ===== UI TEXTURES =====
    AddLoadTask([this]() {
        // ��������� UI �������� ����� TextureRegistry
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "UI");
        // ������������� ��������� ��� ������������������ �������� UI
        // ����� ����� �������� ���������� ��������, ���� �����
    }, "Load Texture: UI", 0.5f);

    // ===== MOUNTAINS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Mountains");
    }, "Load Texture: Mountains", 1.5f);

    // ===== MountainsWater =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "MountainsWater");
    }, "Load Texture: MountainsWater", 1.5f);

    // ===== TREES =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Trees");
    }, "Load Texture: Trees", 1.5f);

    // ===== ROCKS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Rocks");
    }, "Load Texture: Rocks", 1.0f);

    // ===== DECORATIONS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Decorations");
    }, "Load Texture: Decorations", 1.0f);

    // ===== BUILDINGS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Buildings");
    }, "Load Texture: Buildings", 1.5f);

    // ===== ROADS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Roads");
    }, "Load Texture: Roads", 0.5f);

    // ===== UNITS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Units");
    }, "Load Texture: Units", 1.0f);

    // ===== RESOURCE ICONS =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "ResourceIcons");
    }, "Load Texture: ResourceIcons", 0.5f);

    // ===== ATLAS ICONS =====
    AddLoadTask([this]() {
        LoadAtlasOrTexture("ground", "game:\\Media\\Textures\\AtlasTextures\\ground.png");
    }, "Load Texture: Atlas ground", 0.5f);

    AddLoadTask([this]() {
        LoadAtlasOrTexture("icon_tree", "game:\\Media\\Textures\\AtlasTextures\\icon_tree.png");
    }, "Load Texture: Atlas icon_tree", 0.5f);

    AddLoadTask([this]() {
        LoadAtlasOrTexture("icon_mountains", "game:\\Media\\Textures\\AtlasTextures\\icon_mountains.png");
    }, "Load Texture: Atlas icon_mountains", 0.5f);

    AddLoadTask([this]() {
        LoadAtlasOrTexture("icon_mountains_water", "game:\\Media\\Textures\\AtlasTextures\\icon_mountains_water.png");
    }, "Load Texture: Atlas icon_mountains_water", 0.5f);

    AddLoadTask([this]() {
        LoadAtlasOrTexture("icon_rocks", "game:\\Media\\Textures\\AtlasTextures\\icon_rocks.png");
    }, "Load Texture: Atlas icon_rocks", 0.5f);

    AddLoadTask([this]() {
        LoadAtlasOrTexture("icon_menu", "game:\\Media\\Textures\\AtlasTextures\\icon_menu.png");
    }, "Load Texture: Atlas icon_menu", 0.5f);
}
void LoadingScene::AddLoadTask(std::function<void()> task, const std::string& name, float weight)
{
    LoadTask loadTask;
    loadTask.taskFunc = task;
    loadTask.name = name;
    loadTask.weight = weight;
    m_loadTasks.push_back(loadTask);
    m_totalWeight += weight;
}

float LoadingScene::GetTotalProgress() const
{
    if (m_totalWeight <= 0.0f) return 0.0f;
    float progress = m_completedWeight;
    if (m_currentTaskIndex < m_loadTasks.size() && m_totalWeight > 0.0f)
    {
        progress += m_currentTaskProgress * m_loadTasks[m_currentTaskIndex].weight;
    }
    return (progress / m_totalWeight) * 100.0f;
}

const std::string& LoadingScene::GetCurrentTaskName() const
{
    static std::string empty = "";
    if (m_currentTaskIndex < m_loadTasks.size()) {
        return m_loadTasks[m_currentTaskIndex].name;
    }
    return empty;
}

void LoadingScene::ExecuteCurrentTask()
{
    if (m_currentTaskIndex >= m_loadTasks.size()) return;

    LoadTask& task = m_loadTasks[m_currentTaskIndex];
    
    // Execute task only if function is valid
    if (task.taskFunc) {
        task.taskFunc();
    }
    
    // Mark as completed
    m_completedWeight += task.weight;
    m_currentTaskIndex++;
    m_currentTaskProgress = 0.0f;

    if (m_currentTaskIndex < m_loadTasks.size()) {
        m_statusText = m_loadTasks[m_currentTaskIndex].name;
    }
}

void LoadingScene::CreateNextScene()
{
    m_loadingComplete = true;
    
    // After loading completes, switch to target scene
    if (!m_targetScene.empty()) {
        RequestSceneSwitch(m_targetScene);
    }
}

void LoadingScene::Update(float deltaTime)
{
    if (m_loadingComplete) return;

    if (m_currentTaskIndex < m_loadTasks.size())
    {
        ExecuteCurrentTask();
    }
    else
    {
        m_loadingComplete = true;
        CreateNextScene();
    }

    m_loadProgress = GetTotalProgress();
}

void LoadingScene::Render()
{
    // Update screen size
    if (m_renderer && m_renderer->GetDevice()) {
        D3DVIEWPORT9 vp;
        if (SUCCEEDED(m_renderer->GetDevice()->GetViewport(&vp))) {
            m_screenW = static_cast<float>(vp.Width);
            m_screenH = static_cast<float>(vp.Height);
        }
    }

    // Draw loading background if available
    if (m_renderer && m_backgroundTexture.GetTexture()) {
        m_renderer->DrawSingleSprite(&m_backgroundTexture, 0.0f, 0.0f, m_screenW, m_screenH);
    }

    // Draw progress bar background
    float barWidth = 400.0f;
    float barHeight = 20.0f;
    float barX = (m_screenW - barWidth) * 0.5f;
    float barY = m_screenH - 80.0f;

    // Draw progress bar fill based on completion
    float totalProgress = GetTotalProgress() / 100.0f;
    float fillWidth = barWidth * totalProgress;

    // Simple progress bar using colored quads (white fill on dark background)
    // Draw bar background (dark gray)
    if (m_renderer) {
        // Progress bar background - could use a simple texture or DrawSingleSprite with different colors
        // For now, we'll just indicate progress through text
    }
}

}
