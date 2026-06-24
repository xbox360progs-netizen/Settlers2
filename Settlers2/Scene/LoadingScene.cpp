#include "stdafx.h"
#include "LoadingScene.h"
#include "SceneManager.h" // Добавляем этот заголовок
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/Texture.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/RenderLayers.h"
#include "../Graphics/RenderQueue.h"
#include "../Graphics/RenderCommandBuilder.h"
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
    , m_loadStarted(false)
    , m_textureLoader(nullptr)
    , m_renderer(nullptr)
    , m_spriteRenderer(nullptr)
    , m_shaderManager(nullptr)
    , m_binFileManager(nullptr)
    , m_screenW(1280.0f)
    , m_screenH(720.0f)
    , m_progressTexture(nullptr)
    , m_hLoadingThread(NULL)
    , m_targetProgressPercentage(0)
    , m_isLoadComplete(0)
    , m_currentRenderProgress(0.0f)
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
    std::cout.flush();
    std::cout << "[LoadingScene] m_textureLoader = " << (m_textureLoader ? "VALID" : "NULL") << std::endl;
    std::cout.flush();
    std::cout << "[LoadingScene] m_renderer = " << (m_renderer ? "VALID" : "NULL") << std::endl;
    std::cout.flush();
    std::cout << "[LoadingScene] m_spriteRenderer = " << (m_spriteRenderer ? "VALID" : "NULL") << std::endl;
    std::cout.flush();
    std::cout << "[LoadingScene] m_binFileManager = " << (m_binFileManager ? "VALID" : "NULL") << std::endl;
    std::cout.flush();

    // Initialize Xbox 360 async loading variables
    m_targetProgressPercentage = 0;
    m_isLoadComplete = 0;
    m_currentRenderProgress = 0.0f;

    m_currentTaskIndex = 0;
    m_completedWeight = 0.0f;
    m_totalWeight = 0.0f;
    m_currentTaskProgress = 0.0f;
    m_loadProgress = 0.0f;
    m_loadingComplete = false;
    m_loadStarted = false;
    m_statusText = "Loading...";

    // Initialize texture registry device bindings and manifest-based paths
    if (m_renderer) {
        std::cout << "[LoadingScene] Initializing TextureRegistry..." << std::endl;
        std::cout.flush();
        
        // Initialize thread safety first
        TextureRegistry::instance().initThreadSafety();
        
        TextureRegistry::instance().initialize(m_renderer->GetDevice());
        // Set BinFileManager for atlas loading support using static helper
        extern void SetBinFileManagerStatic(BinFileManager* mgr);
        if (m_binFileManager) {
            SetBinFileManagerStatic(m_binFileManager);
        }
        // Main thread should NOT do file I/O or D3D operations
        // All manifest initialization moved to background thread in SetupLoadTasks
    }

    std::cout << "[LoadingScene] TextureRegistry initialized" << std::endl;
    std::cout.flush();

    // Check if target scene is already loaded to skip loading
    SceneManager* mgr = GetSceneManager();
    if (mgr) {
        Scene* target = mgr->GetScene(m_targetScene);
        if (target && target->IsLoaded()) {
            std::cout << "[LoadingScene] Target scene " << m_targetScene << " already loaded, skipping loading." << std::endl;
            std::cout.flush();
            m_loadingComplete = true;
            m_loadStarted = true; // Prevents loading tasks
            return; // Skip rest of Load()
        }
    }

    // Clear previous tasks
    m_loadTasks.clear();

    // Main thread should NOT do D3D operations - background thread will load all textures
    // LoadingScene will use not-found texture until background thread completes
    std::cout << "[LoadingScene] Background texture will be loaded by background thread" << std::endl;
    std::cout.flush();

    // Setup all load tasks using TextureRegistry approach
    std::cout << "[LoadingScene] Setting up load tasks..." << std::endl;
    std::cout.flush();
    SetupLoadTasks();
    std::cout << "[LoadingScene] Load tasks setup complete, total tasks: " << m_loadTasks.size() << std::endl;
    std::cout.flush();

    // Diagnostics
    TextureRegistry::instance().logManifestPathsStatus();

    // Async loading is started from Update() after SceneManager finishes SwitchTo().
    m_loaded = true;
    std::cout << "[LoadingScene] Load() complete - returning to SceneManager" << std::endl;
    std::cout.flush();
}

void LoadingScene::StartAsyncLoading()
{
    if (m_loadStarted) {
        return;
    }

    m_loadStarted = true;
    std::cout << "[LoadingScene] Single-thread loading started" << std::endl;
    std::cout.flush();
}

void LoadingScene::OnExit()
{
    m_loadStarted = false;
    m_loadingComplete = false;
    m_isLoadComplete = 0;
    m_targetProgressPercentage = 0;
    m_currentRenderProgress = 0.0f;
    m_currentTaskIndex = 0;
    m_completedWeight = 0.0f;
    m_totalWeight = 0.0f;
    m_loadTasks.clear();
    m_loaded = false;
}

void LoadingScene::Unload()
{
    // Wait for loading thread to complete if still running
    if (m_hLoadingThread != NULL) {
        WaitForSingleObject(m_hLoadingThread, INFINITE);
        XCloseHandle(m_hLoadingThread);
        m_hLoadingThread = NULL;
    }

    m_loadTasks.clear();
    m_loaded = false;
}

// Xbox 360 static thread function - bridge to instance method
DWORD WINAPI LoadingScene::XboxThreadFunc(LPVOID lpParam)
{
    LoadingScene* pScene = static_cast<LoadingScene*>(lpParam);
    if (pScene) {
        pScene->AsyncLoadResources();
    }
    return 0;
}

// Async loading running on background thread
void LoadingScene::AsyncLoadResources()
{
    std::cout << "[LoadingScene::AsyncLoadResources] Started!" << std::endl;
    std::cout.flush();
    OutputDebugStringA("[LoadingScene::AsyncLoadResources] Started!\n");
    
    // Execute all load tasks
    for (size_t i = 0; i < m_loadTasks.size(); ++i) {
        LoadTask& task = m_loadTasks[i];
        std::cout << "[LoadingScene::AsyncLoadResources] Executing: " << task.name << std::endl;
        std::cout.flush();
        
        char logBuf[256];
        _snprintf(logBuf, sizeof(logBuf), "[LoadingScene::AsyncLoadResources] Executing task %d/%d: %s\n", 
                  (int)i, (int)m_loadTasks.size(), task.name.c_str());
        OutputDebugStringA(logBuf);

        // Update status text
        m_statusText = task.name;

        // Execute task
        if (task.taskFunc) {
            task.taskFunc();
        }

        // Calculate progress percentage (0-100)
        float completedSoFar = m_completedWeight + task.weight;
        float progressPercent = (completedSoFar / m_totalWeight) * 100.0f;

        // Update target progress atomically
        InterlockedExchange(&m_targetProgressPercentage, (LONG)progressPercent);

        // Update completed weight
        m_completedWeight = completedSoFar;

        std::cout << "[LoadingScene::AsyncLoadResources] Task '" << task.name << "' completed (" << (int)progressPercent << "%)" << std::endl;
        _snprintf(logBuf, sizeof(logBuf), "[LoadingScene::AsyncLoadResources] Task '%s' completed (%d%%)\n", 
                  task.name.c_str(), (int)progressPercent);
        OutputDebugStringA(logBuf);
    }

    // Signal completion
    InterlockedExchange(&m_isLoadComplete, 1);
    std::cout << "[LoadingScene::AsyncLoadResources] All tasks completed" << std::endl;
    OutputDebugStringA("[LoadingScene::AsyncLoadResources] *** ALL TASKS COMPLETED ***\n");
    std::cout.flush();
}

void LoadingScene::LoadAtlasOrTexture(const char* name, const char* pngPath)
{
    if (!m_binFileManager || !m_renderer) {
        return;
    }

    char logInit[512];
    _snprintf(logInit, sizeof(logInit), "[LoadingScene] LoadAtlasOrTexture: %s <- %s\n", name, pngPath);
    OutputDebugStringA(logInit);

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
    // ===== MENU TEXTURES (must load first for MenuScene) =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Menu");
        TextureRegistry::instance().getTextureOrLoad("menu_background");
    }, "Load Texture: Menu", 0.5f);

    // ===== LOADING SCREEN TEXTURES =====
    AddLoadTask([this]() {
        TextureRegistry::instance().registerTexturePath("loading_background", "Background/loading_background.png");
        LPDIRECT3DTEXTURE9 bgTex = TextureRegistry::instance().getTextureOrLoad("loading_background");
        LPDIRECT3DTEXTURE9 barTex = TextureRegistry::instance().getTextureOrLoad("progressBarBackground");
        if (bgTex && m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(0, bgTex);
            m_backgroundTexture.SetTexture(bgTex);
        }
        if (barTex && m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(2, barTex);
        }
    }, "Load Texture: Loading Screen", 0.5f);

    // ===== ATLAS TEXTURES (all sprites in one maptiles atlas) =====
    AddLoadTask([this]() {
        LoadAtlasOrTexture("maptiles", "game:\\Media\\Textures\\AtlasTextures\\maptiles.png");
    }, "Load Texture: maptiles", 0.5f);

    // ===== UI ATLAS (cursor, button hints, menu sprites) =====
    AddLoadTask([this]() {
        TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "UI");
        LoadAtlasOrTexture("ui", "game:\\Media\\Textures\\UI\\UI.png");
    }, "Load Texture: UI", 0.5f);

    // ===== ICON ATLAS (resource deposit icons for map editor) =====
    AddLoadTask([this]() {
        LoadAtlasOrTexture("Icon", "game:\\Media\\Textures\\UI\\Icon.png");
    }, "Load Texture: Icon", 0.5f);
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
    if (!m_loadStarted) {
        StartAsyncLoading();
    }

    if (!m_loadingComplete && m_currentTaskIndex < m_loadTasks.size()) {
        LoadTask& task = m_loadTasks[m_currentTaskIndex];
        m_statusText = task.name;

        char logBuf[256];
        _snprintf(logBuf, sizeof(logBuf), "[LoadingScene] Main-thread task %d/%d: %s\n",
                  (int)m_currentTaskIndex, (int)m_loadTasks.size(), task.name.c_str());
        OutputDebugStringA(logBuf);

        ExecuteCurrentTask();

        float progressPercent = (m_totalWeight > 0.0f) ? ((m_completedWeight / m_totalWeight) * 100.0f) : 100.0f;
        InterlockedExchange(&m_targetProgressPercentage, (LONG)progressPercent);

        if (m_currentTaskIndex >= m_loadTasks.size()) {
            InterlockedExchange(&m_isLoadComplete, 1);
            InterlockedExchange(&m_targetProgressPercentage, 100);
            OutputDebugStringA("[LoadingScene] Single-thread loading complete\n");
        }
    }

    // Atomically read target progress (0-100)
    LONG targetProgress = InterlockedExchangeAdd(&m_targetProgressPercentage, 0);
    float targetProgressFloat = (float)targetProgress / 100.0f; // Convert to 0.0-1.0

    // LERP for smooth progress bar movement
    float lerpSpeed = 5.0f; // Speed of interpolation
    m_currentRenderProgress += (targetProgressFloat - m_currentRenderProgress) * lerpSpeed * deltaTime;

    // Clamp to valid range
    if (m_currentRenderProgress < 0.0f) m_currentRenderProgress = 0.0f;
    if (m_currentRenderProgress > 1.0f) m_currentRenderProgress = 1.0f;

    // Check if loading is complete
    LONG isComplete = InterlockedExchangeAdd(&m_isLoadComplete, 0);
    if (!m_loadingComplete && isComplete && m_currentRenderProgress >= 0.99f) {
        OutputDebugStringA("[LoadingScene] Creating next scene...\n");
        CreateNextScene();
    }
}

void LoadingScene::Render(Graphics::RenderQueue* renderQueue)
{
    OutputDebugStringA("[LoadingScene] Render start\n");
	if (!renderQueue) return;

	if (m_renderer && m_renderer->GetDevice()) {
		D3DVIEWPORT9 vp;
		if (SUCCEEDED(m_renderer->GetDevice()->GetViewport(&vp))) {
			m_screenW = static_cast<float>(vp.Width);
			m_screenH = static_cast<float>(vp.Height);
		}
	}

	// Background — re-acquire texture each frame until loaded
	if (!m_backgroundTexture.GetTexture()) {
		LPDIRECT3DTEXTURE9 bgTex = TextureRegistry::instance().getTextureOrLoad("loading_background");
		if (bgTex && m_spriteRenderer) {
			m_spriteRenderer->SetTextureSlot(0, bgTex);
			m_backgroundTexture.SetTexture(bgTex);
		}
	}
	if (m_backgroundTexture.GetTexture()) {
		Graphics::RenderCommandBuilder()
			.Position(0.0f, 0.0f)
			.Size(m_screenW, m_screenH)
			.UV(0.0f, 0.0f, 1.0f, 1.0f)
			.Texture(0)
			.Shader(SHADER_SPRITE)
			.Layer(LAYER_UI)
			.Depth(950)
			.Submit(renderQueue);
	}

	// Progress bar — ensure texture is bound
	if (!m_progressTexture) {
		LPDIRECT3DTEXTURE9 barTex = TextureRegistry::instance().getTextureOrLoad("progressBarBackground");
		if (barTex && m_spriteRenderer) {
			m_spriteRenderer->SetTextureSlot(2, barTex);
			m_progressTexture = barTex;
		}
	}
	if (m_currentRenderProgress > 0.0f) {
		float barWidth = 400.0f;
		float barHeight = 20.0f;
		float barX = (m_screenW - barWidth) * 0.5f;
		float barY = m_screenH - 80.0f;
		float fillWidth = barWidth * m_currentRenderProgress;

		Graphics::RenderCommandBuilder()
			.Position(barX, barY)
			.Size(fillWidth, barHeight)
			.UV(0.0f, 0.0f, m_currentRenderProgress, 1.0f)
			.Texture(2)
			.Shader(SHADER_SPRITE)
			.Layer(LAYER_UI)
			.Depth(0)
			.Submit(renderQueue);
	}
}
}
