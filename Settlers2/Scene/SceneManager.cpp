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
    , m_isSceneReady(false)
    , m_bSceneGraphicsReady(false)
    , m_frameRendered(false)
{
    s_pInstance = this;
}

SceneManager::~SceneManager()
{
    Clear();
}

void SceneManager::AddScene(SceneBase* scene)
{
    if (!scene)
    {
        return;
    }

    // Set this as the scene's manager
    scene->SetSceneManager(this);

    std::map<std::string, SceneBase*>::iterator it = m_scenes.find(scene->GetName());
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
    std::map<std::string, SceneBase*>::iterator it = m_scenes.find(name);
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
    char buf[512];
    _snprintf(buf, sizeof(buf), "[SceneManager::SwitchTo] START name=%s\n", name.c_str());
    OutputDebugStringA(buf);

    // THREAD SAFETY: Lock BEFORE modifying m_currentScene
    OutputDebugStringA("[SceneManager::SwitchTo] Entering critical section\n");
    m_lock.Acquire();
    OutputDebugStringA("[SceneManager::SwitchTo] Critical section entered OK\n");

    // BLOCK render thread on Core 1 - scene is being loaded
    m_isSceneReady = false;
    m_bSceneGraphicsReady = false;
    OutputDebugStringA("[SceneManager::SwitchTo] Blocking render thread\n");

    std::map<std::string, SceneBase*>::iterator it = m_scenes.find(name);
    if (it == m_scenes.end())
    {
        _snprintf(buf, sizeof(buf), "[SceneManager::SwitchTo] ERROR Scene not found=%s\n", name.c_str());
        OutputDebugStringA(buf);
        m_lock.Release();
        return false;
    }
    OutputDebugStringA("[SceneManager::SwitchTo] Scene found\n");

    // Exit current scene OUTSIDE critical section to prevent deadlock
    SceneBase* oldScene = m_currentScene;
    if (oldScene)
    {
        OutputDebugStringA("[SceneManager::SwitchTo] Found old scene, exiting\n");
        m_lock.Release();
        oldScene->OnExit();
        m_lock.Acquire();
        OutputDebugStringA("[SceneManager::SwitchTo] OnExit done\n");
    }

    // Switch to new scene
    SceneBase* newScene = it->second;
    m_currentScene = newScene;
    OutputDebugStringA("[SceneManager::SwitchTo] New scene set\n");

    // UNLOCK before loading to avoid deadlock (Load may access SceneManager/Renderer)
    bool needsLoad = !m_currentScene->IsLoaded();
    _snprintf(buf, sizeof(buf), "[SceneManager::SwitchTo] needsLoad=%d\n", needsLoad?1:0);
    OutputDebugStringA(buf);
    m_lock.Release();
    OutputDebugStringA("[SceneManager::SwitchTo] Left critical section for Load\n");

    // Load OUTSIDE critical section to prevent deadlock
    if (needsLoad)
    {
        OutputDebugStringA("[SceneManager::SwitchTo] Loading scene\n");
        m_currentScene->Load();
        OutputDebugStringA("[SceneManager::SwitchTo] Load done\n");
    }

    // Call OnEnter OUTSIDE critical section to prevent deadlock
    // (OnEnter may access TextureRegistry or other resources with their own locks)
    OutputDebugStringA("[SceneManager::SwitchTo] Calling OnEnter\n");
    m_currentScene->OnEnter();
    OutputDebugStringA("[SceneManager::SwitchTo] OnEnter done\n");
    
    // Re-lock just for setting ready flags
    OutputDebugStringA("[SceneManager::SwitchTo] Re-entering critical section\n");
    m_lock.Acquire();
    OutputDebugStringA("[SceneManager::SwitchTo] Re-entered critical section\n");
    
    _snprintf(buf, sizeof(buf), "[SceneManager::SwitchTo] Switch complete to=%s\n", name.c_str());
    OutputDebugStringA(buf);

    // UNBLOCK render thread - scene is ready
    if (m_spriteRenderer != NULL && m_shaderManager != NULL)
    {
        m_isSceneReady = true;
        m_bSceneGraphicsReady = true;
        OutputDebugStringA("[SceneManager::SwitchTo] Scene ready UNBLOCKED\n");
    }
    else
    {
        OutputDebugStringA("[SceneManager::SwitchTo] WARNING Resources not ready\n");
    }

    OutputDebugStringA("[SceneManager::SwitchTo] Leaving critical section\n");
    m_lock.Release();
    OutputDebugStringA("[SceneManager::SwitchTo] DONE\n");
    return true;
}

SceneBase* SceneManager::GetScene(const std::string& name) const
{
    std::map<std::string, SceneBase*>::const_iterator it = m_scenes.find(name);
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
//        char buf[256];
//        sprintf(buf, "[SceneManager] Render SKIP: scene=%p ready=%d gfx=%d\n",
//                m_currentScene, m_isSceneReady, m_bSceneGraphicsReady);
//        OutputDebugStringA(buf);
        return;
    }

    if (m_renderQueue) {
//        OutputDebugStringA("[SceneManager] Render: BeginFrame on renderQueue\n");
        m_renderQueue->BeginFrame();
    } else {
        OutputDebugStringA("[SceneManager] Render: m_renderQueue IS NULL!\n");
    }

    // Note: Render is called only from main game loop, so no need to lock.
    // Locking here can cause deadlock if scene render accesses TextureRegistry or other locked resources.
    if (m_currentScene) {
//        OutputDebugStringA("[SceneManager] Render: calling scene->Render()\n");
        m_currentScene->Render(m_renderQueue);
    }

    if (m_renderQueue) {
//        char buf[256];
//        sprintf(buf, "[SceneManager] Render: after scene Render, cmdCount=%d\n", m_renderQueue->GetCommandCount());
//        OutputDebugStringA(buf);
    }
}

void SceneManager::Clear()
{
    for (std::map<std::string, SceneBase*>::iterator it = m_scenes.begin();
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

} // namespace Scene
