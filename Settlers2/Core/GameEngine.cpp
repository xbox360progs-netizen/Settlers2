#include "stdafx.h"
#include "GameEngine.h"
#include "../Graphics/BinFileManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/BitmapFont.h"
#include "../Graphics/TextManager.h"
#include "../Scene/SceneManager.h"
#include "../Scene/MenuScene.h"
#include "../Scene/LoadingScene.h"
#include "../Scene/GameScene.h"
#include "../Scene/EditorScene.h"
#include "../Input/InputManager.h"

#include <iostream>
#include <xtl.h>

// Xbox 360 Render Thread Globals
volatile bool g_IsEngineRunning = true;
HANDLE g_hRenderThread = NULL;
ShaderManager* g_pGlobalShaderManager = NULL;
SpriteRenderer* g_pGlobalSpriteRenderer = NULL;

// Render Thread Processor - runs on Core 1 (Thread 2)
DWORD WINAPI RenderThreadProcessor(LPVOID lpParam) {
    OutputDebugStringA("[CORE] Render Pipeline Thread successfully spawned on Core 1 (Thread 2).\n");

    ShaderManager* sm = g_pGlobalShaderManager;
    SpriteRenderer* sr = g_pGlobalSpriteRenderer;

    while (g_IsEngineRunning) {
        // Вызываем асинхронную прокрутку очереди
        sm->ExecuteQueue(sr->GetVertexBuffer(),
                        sr->GetIndexBuffer(),
                        sr->GetVertexDeclaration(),
                        sizeof(SpriteVertex),
                        NULL);

        // Маленькая аппаратная уступка кванта времени Xenon OS, если очередь пустеет
        if (sm->IsQueueEmptyForThisTick()) {
            Sleep(1);
        }
    }
    return 0;
}

// Start Graphics Pipeline with Core 1 binding
void StartGraphicsPipeline(ShaderManager* pShaderManager, SpriteRenderer* pSpriteRenderer) {
    g_pGlobalShaderManager = pShaderManager;
    g_pGlobalSpriteRenderer = pSpriteRenderer;

    // Создаём поток рендеринга
    g_hRenderThread = CreateThread(NULL, 0, RenderThreadProcessor, NULL, 0, NULL);

    // ЖЕСТКАЯ ПРИВЯЗКА К ЯДРУ 1 (Hardware Thread 2)
    // На Xbox 360 это исключает перекидывание потоков планировщиком ОС и сброс L2-кэша
    XSetThreadProcessor(g_hRenderThread, 2);
}

//-------------------------------------------------------------------------------------
// Constructor / Destructor
//-------------------------------------------------------------------------------------
GameEngine::GameEngine()
    : m_running(false)
    , m_initialized(false)
    , m_renderer(NULL)
    , m_spriteRenderer(NULL)
    , m_pShaderManager(NULL)
    , m_bitmapFont(NULL)
    , m_textManager(NULL)
    , m_inputManager(NULL)
    , m_sceneManager(NULL)
{
}

GameEngine::~GameEngine()
{
    Shutdown();
}


//-------------------------------------------------------------------------------------
// Scene Creation
//-------------------------------------------------------------------------------------
void GameEngine::CreateScenes()
{
    // Create LoadingScene
    Scene::LoadingScene* loadingScene = new Scene::LoadingScene();
    if (loadingScene) {
        if (m_textureLoader)
            loadingScene->SetTextureLoader(m_textureLoader);
        if (m_renderer)
            loadingScene->SetRenderer(m_renderer);
        if (m_spriteRenderer)
            loadingScene->SetSpriteRenderer(m_spriteRenderer);
        if (m_binFileManager)
            loadingScene->SetBinFileManager(m_binFileManager);
        // LoadingScene will be used by MenuScene before switching to Game/Editor
    }
    if (loadingScene)
        m_sceneManager->AddScene(loadingScene);

    MenuScene* menuScene = new MenuScene();
    if (menuScene) {
        menuScene->SetTextManager(m_textManager);
        menuScene->SetBinFileManager(m_binFileManager);
        menuScene->Initialize(m_renderer->GetDevice(), m_spriteRenderer, m_renderer, m_inputManager->GetGamepad(), m_textureLoader);
    }
    
    m_sceneManager->AddScene(menuScene);
    
    // Create GameScene (placeholder for now)
    Scene::GameScene* gameScene = new Scene::GameScene();
    if (gameScene) {
        gameScene->Initialize(m_renderer->GetDevice(), m_spriteRenderer);
    }
    m_sceneManager->AddScene(gameScene);
    
    // Create EditorScene (placeholder for now)
    Scene::EditorScene* editorScene = new Scene::EditorScene();
    if (editorScene) {
        editorScene->SetRenderer(m_renderer);
        editorScene->SetSpriteRenderer(m_spriteRenderer);
        editorScene->SetInputManager(m_inputManager);
        editorScene->SetBinFileManager(m_binFileManager);
        editorScene->SetTextManager(m_textManager);
    }
    m_sceneManager->AddScene(editorScene);
    
    m_sceneManager->SwitchTo("MenuScene");
}

//-------------------------------------------------------------------------------------
// Initialize
//-------------------------------------------------------------------------------------
bool GameEngine::Initialize()
{
    if (m_initialized)
    {
        return true;
    }

    m_renderer = new Renderer();
    HRESULT hr = m_renderer->Initialize();
    if (FAILED(hr))
    {
        std::cerr << "[GameEngine] Failed to initialize Renderer" << std::endl;
        return false;
    }

    // Create shader manager after renderer has device
    m_pShaderManager = new ShaderManager();
    m_pShaderManager->Initialize(m_renderer->GetDevice());
    m_pShaderManager->Init();

    // Pass shader manager to renderer
    m_renderer->SetShaderManager(m_pShaderManager);

    m_spriteRenderer = new SpriteRenderer();
    hr = m_spriteRenderer->Initialize(m_renderer->GetDevice(), m_renderer->GetShaderManager());
    if (FAILED(hr))
    {
        std::cerr << "[GameEngine] Failed to initialize SpriteRenderer" << std::endl;
        return false;
    }

    m_inputManager = new Input::InputManager();
    if (!m_inputManager->Initialize(NULL, m_renderer->GetDevice()))
    {
        std::cerr << "[GameEngine] Failed to initialize InputManager" << std::endl;
        return false;
    }

	m_bitmapFont = new BitmapFont(m_renderer->GetDevice());
	m_bitmapFont->Init(m_renderer, m_renderer->GetShaderManager());
	if (!m_bitmapFont->LoadFromFile(L"game:\\Media\\Fonts\\debug_font.fnt"))
	{
		OutputDebugStringA("[GameEngine::Initialize] Warning: Failed to load bitmap font file\n");
	}

    // CRITICAL: Set SpriteRenderer on Renderer BEFORE creating TextManager
    m_renderer->SetSpriteRenderer(m_spriteRenderer);
    OutputDebugStringA("[GameEngine::Initialize] Set SpriteRenderer on Renderer\n");

    m_textManager = new TextManager(m_bitmapFont, 1280.0f, 720.0f, m_renderer->GetSpriteRenderer());
    m_textManager->Init(m_renderer, m_renderer->GetShaderManager());
    
    // XBOX 360 CRITICAL: Load font texture into TextManager
    if (m_bitmapFont->GetTexture()) {
        m_textManager->SetFontAtlas(FONT_MENU, m_bitmapFont->GetTexture());
        OutputDebugStringA("[GameEngine::Initialize] Font texture loaded into TextManager\n");
    } else {
        OutputDebugStringA("[GameEngine::Initialize] ERROR: Font texture is NULL\n");
    }
    m_binFileManager = new BinFileManager();
    m_binFileManager->SetDevice(m_renderer->GetDevice());
    m_textureLoader = new TextureLoader(m_renderer->GetDevice());

    // Initialize TextureRegistry with the device
    TextureRegistry::instance().initialize(m_renderer->GetDevice());

    m_sceneManager = new Scene::SceneManager();
    
    // Set up queue-based rendering pipeline
    m_sceneManager->SetShaderManager(m_renderer->GetShaderManager());
    m_sceneManager->SetSpriteRenderer(m_spriteRenderer);

    CreateScenes();

    // Xbox 360: Start render thread on Core 1
    StartGraphicsPipeline(m_pShaderManager, m_spriteRenderer);
    OutputDebugStringA("[GameEngine::Initialize] Render thread started on Core 1\n");

    m_initialized = true;
    std::cout << "[GameEngine] Initialized successfully" << std::endl;
    return true;
}

//-------------------------------------------------------------------------------------
// Shutdown
//-------------------------------------------------------------------------------------
void GameEngine::Shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    if (m_sceneManager)
    {
        m_sceneManager->Clear();
        delete m_sceneManager;
        m_sceneManager = NULL;
    }

    if (m_spriteRenderer)
    {
        m_spriteRenderer->Shutdown();
        delete m_spriteRenderer;
        m_spriteRenderer = NULL;
    }

    if (m_textManager)
    {
        delete m_textManager;
        m_textManager = nullptr;
    }

    if (m_binFileManager)
    {
        delete m_binFileManager;
        m_binFileManager = nullptr;
    }

    if (m_bitmapFont)
    {
        delete m_bitmapFont;
        m_bitmapFont = NULL;
    }

    if (m_pShaderManager)
    {
        m_pShaderManager->Shutdown();
        delete m_pShaderManager;
        m_pShaderManager = NULL;
    }

    if (m_inputManager)
    {
        delete m_inputManager;
        m_inputManager = NULL;
    }

    if (m_renderer)
    {
        m_renderer->Shutdown();
        delete m_renderer;
        m_renderer = NULL;
    }

    m_initialized = false;
    std::cout << "[GameEngine] Shutdown complete" << std::endl;
}

void GameEngine::ProcessSceneRequests()
{
    Scene::Scene* currentScene = m_sceneManager->GetCurrentScene();
    if (!currentScene)
    {
        return;
    }

    if (currentScene->IsExitRequested())
    {
        m_running = false;
        return;
    }

    if (currentScene->HasPendingSceneSwitch())
    {
        std::string nextScene = currentScene->GetPendingSceneName();
        currentScene->ClearPendingSceneSwitch();
        if (!nextScene.empty())
        {
            m_sceneManager->SwitchTo(nextScene);
        }
    }
}

void GameEngine::Update(float deltaTime)
{
    if (m_inputManager)
    {
        m_inputManager->Update();
    }

    if (m_sceneManager)
    {
        m_sceneManager->Update(deltaTime);
    }

    ProcessSceneRequests();
}

void GameEngine::Render()
{
    if (!m_renderer || !m_sceneManager)
    {
        return;
    }

    m_renderer->BeginFrame();
    m_sceneManager->Render();
    m_renderer->EndFrame();
}

void GameEngine::Run()
{
    if (!m_initialized)
    {
        std::cerr << "[GameEngine] Cannot run - not initialized" << std::endl;
        return;
    }

    m_running = true;
    DWORD lastTime = GetTickCount();

    std::cout << "[GameEngine] Entering main loop" << std::endl;

    while (m_running)
    {
        DWORD currentTime = GetTickCount();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (deltaTime < 0.001f) deltaTime = 0.016f;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        Update(deltaTime);
        Render();

        Sleep(16);
    }

    std::cout << "[GameEngine] Exiting main loop" << std::endl;
}
