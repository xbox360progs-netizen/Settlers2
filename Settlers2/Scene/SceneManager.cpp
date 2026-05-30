#include "stdafx.h"
#include "SceneManager.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/RenderFrame.h"
#include "../Graphics/RenderTypes.h"
#include "../Graphics/RenderQueue.h"
#include "../Scene/MenuScene.h"
#include <iostream>
#include <assert.h>

// Disable all debug logs
#ifdef DISABLE_RENDER_LOGS
#define OutputDebugStringA(...) do { } while(0)
#endif

namespace Scene {

SceneManager* SceneManager::s_pInstance = NULL;

SceneManager::SceneManager()
    : m_currentScene(NULL)
    , m_shaderManager(NULL)
    , m_spriteRenderer(NULL)
    , m_renderer(NULL)
    , m_renderFrame(NULL)
    , m_renderQueue(NULL)
#ifdef _XBOX
    , m_pAsyncCall(NULL)
    , m_pCommandBuffer(NULL)
    , m_pRecordCommandBuffer(NULL)
#endif
    , m_isSceneReady(false)
    , m_bSceneGraphicsReady(false)
    , m_frameRendered(false)
{
    InitializeCriticalSection(&m_cs);
    s_pInstance = this;
}

SceneManager::~SceneManager()
{
#ifdef _XBOX
    // Clean up async command buffer objects
    if (m_pAsyncCall) {
        m_pAsyncCall->Release();
        m_pAsyncCall = NULL;
    }
    if (m_pCommandBuffer) {
        m_pCommandBuffer->Release();
        m_pCommandBuffer = NULL;
    }
#endif
    DeleteCriticalSection(&m_cs);
    Clear();
}

void SceneManager::AddScene(Scene* scene)
{
    if (!scene)
    {
        return;
    }

    // Set this as the scene's manager
    scene->SetSceneManager(this);

    std::map<std::string, Scene*>::iterator it = m_scenes.find(scene->GetName());
    if (it != m_scenes.end())
    {
        // Сцена с таким именем уже есть — удаляем старую
        delete it->second;
        it->second = scene;
    }
    else
    {
        m_scenes[scene->GetName()] = scene;
    }
}

void SceneManager::RemoveScene(const std::string& name)
{
    std::map<std::string, Scene*>::iterator it = m_scenes.find(name);
    if (it != m_scenes.end())
    {
        if (m_currentScene == it->second)
        {
            m_currentScene->OnExit();
            m_currentScene = NULL;
        }
        delete it->second;
        m_scenes.erase(it);
    }
}

bool SceneManager::SwitchTo(const std::string& name)
{
    std::cout << "[SceneManager] Switching to scene: " << name << std::endl;

    // THREAD SAFETY: Lock BEFORE modifying m_currentScene
    EnterCriticalSection(&m_cs);

    // BLOCK render thread on Core 1 - scene is being loaded
    m_isSceneReady = false;
    m_bSceneGraphicsReady = false;
    OutputDebugStringA("[SceneManager] Blocking render thread - scene loading\n");

    std::map<std::string, Scene*>::iterator it = m_scenes.find(name);
    if (it == m_scenes.end())
    {
        std::cout << "[SceneManager] ERROR: Scene not found: " << name << std::endl;
        LeaveCriticalSection(&m_cs);
        return false;
    }

    // Exit current scene
    if (m_currentScene)
    {
        std::cout << "[SceneManager] Exiting current scene: " << m_currentScene->GetName() << std::endl;
        m_currentScene->OnExit();
    }

    // Enter new scene (keep reference only, load outside critical section)
    Scene* newScene = it->second;
    m_currentScene = newScene;
    std::cout << "[SceneManager] Entering new scene: " << m_currentScene->GetName() << std::endl;

    // UNLOCK before loading to avoid deadlock (Load may access SceneManager/Renderer)
    bool needsLoad = !m_currentScene->IsLoaded();
    LeaveCriticalSection(&m_cs);
    std::cout << "[SceneManager] Critical section unlocked, needsLoad=" << (needsLoad?"true":"false") << std::endl;
    std::cout.flush();

    // Load OUTSIDE critical section to prevent deadlock
    if (needsLoad)
    {
        std::cout << "[SceneManager] Calling Load() outside critical section..." << std::endl;
        std::cout.flush();
        m_currentScene->Load();
        std::cout << "[SceneManager] Load() returned" << std::endl;
        std::cout.flush();
    }

    // Re-lock for finalization
    EnterCriticalSection(&m_cs);
    std::cout << "[SceneManager] About to call OnEnter()" << std::endl;
    std::cout.flush();
    OutputDebugStringA("[SceneManager] About to call OnEnter()\n");
    m_currentScene->OnEnter();
    std::cout << "[SceneManager] OnEnter() returned" << std::endl;
    std::cout.flush();
    OutputDebugStringA("[SceneManager] OnEnter() returned\n");
    
    std::cout << "[SceneManager] Switch complete to: " << name << std::endl;

    // UNBLOCK render thread - scene is ready
    if (m_spriteRenderer != NULL && m_shaderManager != NULL)
    {
        m_isSceneReady = true;
        m_bSceneGraphicsReady = true;
        OutputDebugStringA("[SceneManager] Unblocking render thread - scene ready\n");
    }
    else
    {
        OutputDebugStringA("[SceneManager] WARNING: Resources not ready\n");
    }

    LeaveCriticalSection(&m_cs);
    return true;
}

Scene* SceneManager::GetScene(const std::string& name) const
{
    std::map<std::string, Scene*>::const_iterator it = m_scenes.find(name);
    if (it != m_scenes.end())
    {
        return it->second;
    }
    return NULL;
}

const std::string& SceneManager::GetCurrentSceneName() const
{
    static const std::string empty;
    if (m_currentScene)
    {
        return m_currentScene->GetName();
    }
    return empty;
}

void SceneManager::Update(float deltaTime)
{
    // GUARD: If scene not ready or null, skip update to prevent 0xC0000005
    if (m_currentScene == nullptr || !m_isSceneReady) {
        return;
    }
    
    m_currentScene->Update(deltaTime);
}

void SceneManager::Render()
{
    if (m_frameRendered) {
        OutputDebugStringA("[SceneManager] Render SKIP: m_frameRendered==true\n");
        return;
    }
    m_frameRendered = true;

    if (!m_currentScene || !m_isSceneReady || !m_bSceneGraphicsReady) {
        char buf[256];
        sprintf(buf, "[SceneManager] Render SKIP: scene=%p ready=%d gfx=%d\n",
                m_currentScene, m_isSceneReady, m_bSceneGraphicsReady);
        OutputDebugStringA(buf);
        return;
    }

    if (m_renderQueue) {
        OutputDebugStringA("[SceneManager] Render: BeginFrame on renderQueue\n");
        m_renderQueue->BeginFrame();
    } else {
        OutputDebugStringA("[SceneManager] Render: m_renderQueue IS NULL!\n");
    }

    EnterCriticalSection(&m_cs);
    if (m_currentScene) {
        OutputDebugStringA("[SceneManager] Render: calling scene->Render()\n");
        m_currentScene->Render(m_renderQueue);
    }
    LeaveCriticalSection(&m_cs);

    if (m_renderQueue) {
        char buf[256];
        sprintf(buf, "[SceneManager] Render: after scene Render, cmdCount=%d\n", m_renderQueue->GetCommandCount());
        OutputDebugStringA(buf);
    }
}

void SceneManager::Clear()
{
    for (std::map<std::string, Scene*>::iterator it = m_scenes.begin();
         it != m_scenes.end(); ++it)
    {
        if (it->second->IsLoaded())
        {
            it->second->Unload();
        }
        delete it->second;
    }
    m_scenes.clear();
    m_currentScene = NULL;
}

void SceneManager::RenderOverlay()
{
    if (!m_currentScene || !m_isSceneReady || !m_bSceneGraphicsReady) {
        return;
    }
    m_currentScene->RenderOverlay();
}

void SceneManager::ResetFrameRendered() {
    m_frameRendered = false;
}

#ifdef _XBOX
void SceneManager::InitializeAsyncCommandBuffer(LPDIRECT3DDEVICE9 pDevice)
{
    if (!pDevice) {
        OutputDebugStringA("[SceneManager] ERROR: NULL device passed to InitializeAsyncCommandBuffer\n");
        return;
    }

    // 1. Create command buffer for sprite rendering (64KB should be sufficient)
    HRESULT hr = pDevice->CreateCommandBuffer(64 * 1024, 0, &m_pCommandBuffer);
    if (FAILED(hr)) {
        char errBuf[256];
        sprintf(errBuf, "[SceneManager] ERROR: CreateCommandBuffer failed with HRESULT=0x%08X\n", hr);
        OutputDebugStringA(errBuf);
        return;
    }
    OutputDebugStringA("[SceneManager] Command buffer created successfully\n");

    // 2. Create async command buffer call
    // NULL for inherit/persist tags to use standard render state
    hr = pDevice->CreateAsyncCommandBufferCall(
        NULL, // pInheritTags
        NULL, // pPersistTags
        1,    // NumSegments (1 segment = fastest, no GPU delays)
        0,    // Flags
        &m_pAsyncCall
    );

    if (FAILED(hr)) {
        char errBuf[256];
        sprintf(errBuf, "[SceneManager] ERROR: CreateAsyncCommandBufferCall failed with HRESULT=0x%08X\n", hr);
        OutputDebugStringA(errBuf);
        // Clean up command buffer if async call creation failed
        if (m_pCommandBuffer) {
            m_pCommandBuffer->Release();
            m_pCommandBuffer = NULL;
        }
        return;
    }
    OutputDebugStringA("[SceneManager] Async command buffer call created successfully\n");
}
#endif

} // namespace Scene
