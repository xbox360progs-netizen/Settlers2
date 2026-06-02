#include "stdafx.h"
#include "GameScene.h"
#include "../Graphics/RenderQueue.h"
#include "../World/Map.h"
#include "../Logic/EconomyManager.h"
#include "../World/CarrierManager.h"
#include "../Logic/AISystem.h"

namespace Scene {

bool GameScene::g_sceneRunning = false;

GameScene::GameScene()
    : Scene("Game")
    , m_map(NULL)
    , m_economyManager(NULL)
    , m_carrierManager(NULL)
    , m_aiSystem(NULL)
    , m_logicThread(NULL)
    , m_aiThread(NULL)
{
}

GameScene::~GameScene()
{
}

void GameScene::Initialize(IDirect3DDevice9* device, Graphics::SpriteRenderer* spriteRenderer) {
    m_map = new World::Map(20, 20, 40, 80);
    m_economyManager = new Logic::EconomyManager();
    m_carrierManager = new World::CarrierManager();
    m_aiSystem = new Logic::AISystem(0, m_map, m_economyManager);
}

void GameScene::Load()
{
    g_sceneRunning = true;
    m_logicThread = CreateThread(NULL, 0, LogicThreadFunction, this, 0, NULL);
    m_aiThread = CreateThread(NULL, 0, AIThreadFunction, this, 0, NULL);
    m_loaded = true;
}

void GameScene::Unload()
{
    g_sceneRunning = false;
    if (m_logicThread) {
        WaitForSingleObject(m_logicThread, INFINITE);
        CloseHandle(m_logicThread);
        m_logicThread = NULL;
    }
    if (m_aiThread) {
        WaitForSingleObject(m_aiThread, INFINITE);
        CloseHandle(m_aiThread);
        m_aiThread = NULL;
    }
    
    // Cleanup systems
    delete m_map;
    delete m_economyManager;
    delete m_carrierManager;
    delete m_aiSystem;
    
    m_loaded = false;
}

DWORD WINAPI GameScene::LogicThreadFunction(LPVOID lpParam) {
    GameScene* scene = (GameScene*)lpParam;
#ifdef _XBOX
    XSetThreadProcessor(GetCurrentThread(), 1);
#endif
    while (g_sceneRunning) {
        if (scene && scene->m_economyManager && scene->m_carrierManager) {
            scene->m_economyManager->Update(scene->m_carrierManager);
            scene->m_carrierManager->Update(0.016f);
        }
        Sleep(16);
    }
    return 0;
}

DWORD WINAPI GameScene::AIThreadFunction(LPVOID lpParam) {
    GameScene* scene = (GameScene*)lpParam;
#ifdef _XBOX
    XSetThreadProcessor(GetCurrentThread(), 2);
#endif
    while (g_sceneRunning) {
        if (scene && scene->m_aiSystem) {
            scene->m_aiSystem->Update(0.1f);
        }
        Sleep(100);
    }
    return 0;
}

void GameScene::Update(float deltaTime)
{
    // Graphic engine updates or UI updates here
}

void GameScene::Render(Graphics::RenderQueue* renderQueue)
{
    (void)renderQueue;
}

} // namespace Scene
