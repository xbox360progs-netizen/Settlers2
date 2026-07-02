#include "stdafx.h"
#include "GameEngine.h"
#include "../Graphics/BinFileManager.h"
#include "../Graphics/TextureLoader.h"
#include "../Graphics/TextureRegistry.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderFrame.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/BitmapFont.h"
#include "../Graphics/TextManager.h"
#include "../Graphics/RenderQueue.h"
#include "../Scene/SceneManager.h"
#include "../Scene/MenuScene.h"
#include "../Scene/MenuCommandDispatcher.h"
#include "../Scene/LoadingScene.h"
#include "../Scene/GameScene.h"
#include "../Scene/EditorScene.h"
#include "../Input/InputManager.h"

#include <iostream>
#include <xtl.h>
#include <stdio.h>

extern void SetBinFileManagerStatic(BinFileManager* mgr);

volatile bool g_IsEngineRunning = true;

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

	char buf[256];
    sprintf(buf, "[GameEngine::CreateScenes] BEFORE SetRenderer: m_spriteRenderer=%p, vtable=%p\n", 
            m_spriteRenderer, m_spriteRenderer ? *(void***)m_spriteRenderer : nullptr);
    OutputDebugStringA(buf);

    if (loadingScene)
        m_sceneManager->AddScene(loadingScene);

    OutputDebugStringA("[GameEngine::CreateScenes] Creating MenuScene...\n");
    MenuScene* menuScene = new MenuScene();
    if (menuScene) {
        menuScene->SetTextManager(m_textManager);
        menuScene->SetBinFileManager(m_binFileManager);
		menuScene->SetRenderer(m_spriteRenderer, m_renderer);
        OutputDebugStringA("[GameEngine::CreateScenes] BEFORE menuScene->Initialize\n");
        menuScene->Initialize(m_renderer->GetDevice(), m_spriteRenderer, m_renderer, m_inputManager->GetGamepad(), m_textureLoader);
        OutputDebugStringA("[GameEngine::CreateScenes] AFTER menuScene->Initialize\n");
    }
    
    OutputDebugStringA("[GameEngine::CreateScenes] BEFORE AddScene(menuScene)\n");
    m_sceneManager->AddScene(menuScene);
    // Wire MenuCommandDispatcher
    {
        Scene::MenuCommandDispatcher* dispatcher = new Scene::MenuCommandDispatcher(m_sceneManager);
        menuScene->SetDispatcher(dispatcher);
        OutputDebugStringA("[GameEngine::CreateScenes] MenuCommandDispatcher wired\n");
    }

    OutputDebugStringA("[GameEngine::CreateScenes] AFTER AddScene(menuScene)\n");
    
    // Create GameScene (placeholder for now)
    OutputDebugStringA("[CreateScenes] BEFORE GameScene creation\n");
    Scene::GameScene* gameScene = new Scene::GameScene();
    if (gameScene) {
        OutputDebugStringA("[CreateScenes] BEFORE GameScene::Initialize\n");
        gameScene->Initialize(m_renderer->GetDevice(), m_spriteRenderer);
        OutputDebugStringA("[CreateScenes] AFTER GameScene::Initialize\n");
        gameScene->SetRenderer(m_renderer);
        gameScene->SetInputManager(m_inputManager);
        gameScene->SetTextManager(m_textManager);
        OutputDebugStringA("[CreateScenes] AFTER SetRenderer\n");
    }
    OutputDebugStringA("[CreateScenes] BEFORE AddScene(gameScene)\n");
    m_sceneManager->AddScene(gameScene);
    OutputDebugStringA("[CreateScenes] AFTER AddScene(gameScene)\n");
    
    // Create EditorScene (placeholder for now)
    OutputDebugStringA("[CreateScenes] BEFORE EditorScene creation\n");
    Scene::EditorScene* editorScene = new Scene::EditorScene();
    if (editorScene) {
        editorScene->SetRenderer(m_renderer);
        editorScene->SetSpriteRenderer(m_spriteRenderer);
        editorScene->SetInputManager(m_inputManager);
        editorScene->SetBinFileManager(m_binFileManager);
        editorScene->SetTextManager(m_textManager);
    }
    m_sceneManager->AddScene(editorScene);
    
    OutputDebugStringA("[CreateScenes] BEFORE SwitchTo\n");
    m_sceneManager->SwitchTo("MenuScene");
    OutputDebugStringA("[CreateScenes] AFTER SwitchTo\n");
}

//-------------------------------------------------------------------------------------
// Initialize
//-------------------------------------------------------------------------------------
bool GameEngine::Initialize()
{
    setvbuf(stdout, NULL, _IONBF, 0);
    OutputDebugStringA("[GameEngine::Initialize] START\n");

    if (m_initialized)
    {
        return true;
    }

    OutputDebugStringA("[GameEngine::Initialize] Creating ShaderManager\n");
    m_pShaderManager = new ShaderManager();

    OutputDebugStringA("[GameEngine::Initialize] Creating Renderer\n");
    m_renderer = new Renderer();
    m_renderer->SetShaderManager(m_pShaderManager);
    HRESULT hr = m_renderer->Initialize();
    if (FAILED(hr))
    {
        OutputDebugStringA("[GameEngine::Initialize] FAILED: Renderer initialization\n");
        return false;
    }

    OutputDebugStringA("[GameEngine::Initialize] Initializing ShaderManager\n");
    m_pShaderManager->Initialize(m_renderer->GetDevice());
    m_pShaderManager->Init();
    OutputDebugStringA("[GameEngine::Initialize] ShaderManager initialized\n");

    m_spriteRenderer = new SpriteRenderer();
    hr = m_spriteRenderer->Initialize(m_renderer->GetDevice(), m_renderer->GetShaderManager());
    if (FAILED(hr))
    {
        OutputDebugStringA("[GameEngine] Failed to initialize SpriteRenderer\n");
        return false;
    }


	m_renderer->SetSpriteRenderer(m_spriteRenderer); 

    m_inputManager = new Input::InputManager();
    if (!m_inputManager->Initialize(NULL, m_renderer->GetDevice()))
    {
        OutputDebugStringA("[GameEngine] Failed to initialize InputManager\n");
        return false;
    }

	m_bitmapFont = new BitmapFont(m_renderer->GetDevice());
	m_bitmapFont->Init(m_renderer, m_renderer->GetShaderManager());
	if (!m_bitmapFont->LoadFromFile(L"game:\\Media\\Fonts\\English.fnt"))
	{
		OutputDebugStringA("[GameEngine::Initialize] Warning: Failed to load bitmap font file\n");
	}

    m_sceneManager = new Scene::SceneManager();

    m_sceneManager->SetShaderManager(m_renderer->GetShaderManager());
    m_sceneManager->SetSpriteRenderer(m_spriteRenderer);
    m_sceneManager->SetRenderer(m_renderer);
    m_sceneManager->SetRenderFrame(m_renderer->GetRenderFrame());

    Graphics::RenderQueue* renderQueue = m_renderer->GetRenderQueue();
    m_sceneManager->SetRenderQueue(renderQueue);

    m_textManager = new TextManager(m_bitmapFont, 1280.0f, 720.0f, renderQueue);

    if (m_bitmapFont->GetTexture()) {
        m_textManager->SetFontAtlas(FONT_MENU, m_bitmapFont->GetTexture());

        if (m_spriteRenderer) {
            m_spriteRenderer->SetTextureSlot(15, m_bitmapFont->GetTexture());
        }

        OutputDebugStringA("[GameEngine::Initialize] Font texture loaded into TextManager\n");
    } else {
        OutputDebugStringA("[GameEngine::Initialize] ERROR: Font texture is NULL\n");
    }
    m_binFileManager = new BinFileManager();
    m_binFileManager->SetDevice(m_renderer->GetDevice());
    m_textureLoader = new TextureLoader(m_renderer->GetDevice());

    // Initialize TextureRegistry before any scene can query it.
    OutputDebugStringA("[GameEngine::Initialize] Initializing TextureRegistry thread safety...\n");
    TextureRegistry::instance().initThreadSafety();
    OutputDebugStringA("[GameEngine::Initialize] Initializing TextureRegistry device...\n");
    TextureRegistry::instance().initialize(m_renderer->GetDevice());
    SetBinFileManagerStatic(m_binFileManager);
    OutputDebugStringA("[GameEngine::Initialize] Loading TextureRegistry manifest...\n");
    TextureRegistry::instance().initializeFromManifest("game:\\Media\\Config\\textures.ini", "Menu");
    OutputDebugStringA("[GameEngine::Initialize] TextureRegistry initialized\n");

    CreateScenes();

    m_initialized = true;
    OutputDebugStringA("[GameEngine] Initialized successfully\n");
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
    OutputDebugStringA("[GameEngine] Shutdown complete\n");
}

void GameEngine::ProcessSceneRequests()
{
    Scene::SceneBase* currentScene = m_sceneManager->GetCurrentScene();
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
    // GUARD: Wait for scene to be ready before updating
    if (m_sceneManager && !m_sceneManager->IsSceneReady()) {
        return;
    }

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

    if (!m_sceneManager->IsSceneReady()) {
        return;
    }

    m_sceneManager->ResetFrameRendered();
    m_renderer->BeginFrame();

    RenderFrame* renderFrame = m_renderer->GetRenderFrame();
    if (renderFrame) {
        renderFrame->BeginFrame();
        m_sceneManager->Render();
        renderFrame->Execute();
        m_sceneManager->RenderOverlay();
        renderFrame->EndFrame();
    } else {
        m_sceneManager->Render();
    }

    m_renderer->EndFrame();
}

void GameEngine::Run()
{
    if (!m_initialized)
    {
        OutputDebugStringA("[GameEngine] Cannot run - not initialized\n");
        return;
    }

#ifdef _XBOX
    XSetThreadProcessor(GetCurrentThread(), 0);
#endif

    m_running = true;
    DWORD lastTime = GetTickCount();

//    OutputDebugStringA("[GameEngine] Entering main loop\n");

    while (m_running)
    {
#ifdef _XBOX
//        OutputDebugStringA("[Loop] 1 - top of loop\n");
        DWORD currentTime = GetTickCount();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        if (deltaTime < 0.001f) deltaTime = 0.016f;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

//        OutputDebugStringA("[Loop] 2 - before input update\n");
        if (m_inputManager) m_inputManager->Update();
//        OutputDebugStringA("[Loop] 3 - after input update\n");
        if (m_sceneManager) m_sceneManager->Update(deltaTime);
//        OutputDebugStringA("[Loop] 4 - after scene update\n");

        ProcessSceneRequests();
//        OutputDebugStringA("[Loop] 5 - after process requests\n");
        if (m_sceneManager && m_sceneManager->IsSceneReady()) {
//            OutputDebugStringA("[Loop] 6 - scene ready, rendering\n");
            m_sceneManager->ResetFrameRendered();
            m_renderer->BeginFrame();
//            OutputDebugStringA("[Loop] 7 - after BeginFrame\n");

            RenderFrame* renderFrame = m_renderer->GetRenderFrame();
            if (renderFrame) {
//                OutputDebugStringA("[Loop] 8 - renderFrame BeginFrame\n");
                renderFrame->BeginFrame();
//                OutputDebugStringA("[Loop] 9 - before SceneManager::Render\n");
                m_sceneManager->Render();
//                OutputDebugStringA("[Loop] 10 - after Render, before Execute\n");
                renderFrame->Execute();
//                OutputDebugStringA("[Loop] 11 - after Execute, before RenderOverlay\n");
                m_sceneManager->RenderOverlay();
//                OutputDebugStringA("[Loop] 12 - after RenderOverlay, EndFrame\n");
                renderFrame->EndFrame();
            } else {
                OutputDebugStringA("[Loop] 8b - no renderFrame, calling Render directly\n");
                m_sceneManager->Render();
            }

//            OutputDebugStringA("[Loop] 13 - before EndFrame\n");
            m_renderer->EndFrame();
//            OutputDebugStringA("[Loop] 14 - after EndFrame\n");
        } else {
            OutputDebugStringA("[Loop] 6b - scene NOT ready\n");
        }
        
        Sleep(16);
#else
        DWORD currentTime = GetTickCount();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;

        if (deltaTime < 0.001f) deltaTime = 0.016f;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        Update(deltaTime);
        Render();
#endif

        Sleep(16);
    }

OutputDebugStringA("[GameEngine] Exiting main loop\n");

    if (m_renderer) {
        m_renderer->Shutdown();
    }

#ifdef _XBOX
    OutputDebugStringA("[GameEngine] Safe shutdown. Returning to Xbox Dashboard...\n");
    // XLaunchNewImage requires Xbox 360 XDK - use XEX loader alternative
    XSetThreadProcessor(GetCurrentThread(), 0);
    Sleep(1000);
#endif
}
