#include "stdafx.h"
#include "SceneManager.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/SpriteRenderer.h"
#include <iostream>

// Disable all debug logs
#ifdef DISABLE_RENDER_LOGS
#define OutputDebugStringA(...) do { } while(0)
#endif

namespace Scene {

SceneManager::SceneManager()
    : m_currentScene(NULL)
    , m_shaderManager(NULL)
    , m_spriteRenderer(NULL)
{
}

SceneManager::~SceneManager()
{
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
    
    std::map<std::string, Scene*>::iterator it = m_scenes.find(name);
    if (it == m_scenes.end())
    {
        std::cout << "[SceneManager] ERROR: Scene not found: " << name << std::endl;
        return false;
    }

    // Выходим из текущей сцены
    if (m_currentScene)
    {
        std::cout << "[SceneManager] Exiting current scene: " << m_currentScene->GetName() << std::endl;
        m_currentScene->OnExit();
    }

    // Входим в новую
    m_currentScene = it->second;
    std::cout << "[SceneManager] Entering new scene: " << m_currentScene->GetName() << std::endl;

    // Загружаем если ещё не загружена
    if (!m_currentScene->IsLoaded())
    {
        std::cout << "[SceneManager] Loading scene: " << m_currentScene->GetName() << std::endl;
        m_currentScene->Load();
    }

    m_currentScene->OnEnter();
    std::cout << "[SceneManager] Switch complete to: " << name << std::endl;
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
    if (m_currentScene)
    {
        m_currentScene->Update(deltaTime);
    }
}

void SceneManager::Render()
{
    if (!m_currentScene)
    {
        return;
    }

    // === MASTER LOOP RENDERING PIPELINE ===
    
    // Step 0: Begin frame and clear screen (CRITICAL!)
    // Note: BeginFrame/EndFrame will be called by GameEngine, not here
    // SceneManager only handles scene rendering and command execution
    
    // Step 1: Clear previous commands from ShaderManager
    if (m_shaderManager)
    {
        m_shaderManager->ClearQueue();
    }

    // Step 2: RECORD - Collect all render commands from scene
    // The scene will call SpriteRenderer which submits commands to the queue
    // Nobody draws anything at this stage!
    
    // Reset vertex count at start of frame
    if (m_spriteRenderer) {
        m_spriteRenderer->ResetVertexCount();
        printf("[SceneManager] Called ResetVertexCount\n");
    }
    
    printf("[SceneManager] About to call m_currentScene->Render()\n");
    OutputDebugStringA("[SceneManager::Render] Calling m_currentScene->Render()...\n");
    m_currentScene->Render();
    OutputDebugStringA("[SceneManager::Render] m_currentScene->Render() returned\n");

    // Step 3: SORT - Sort commands by zOrder, shader, and texture (critical for Xbox 360 performance)
    OutputDebugStringA("[SceneManager::Render] Calling SortQueue()...\n");
    if (m_shaderManager)
    {
        m_shaderManager->SortQueue();
    }
    OutputDebugStringA("[SceneManager::Render] SortQueue() returned\n");

    OutputDebugStringA("[SceneManager::Render] About to Execute Queue...\n");

    // Step 4: EXECUTE - Execute all commands in sorted order (final render pass)
    if (m_shaderManager && m_spriteRenderer)
    {
        // NOTE: Flush() is called by SpriteRenderer::End() during scene rendering
        // No need to call here again
        
        // Get vertex buffer and index buffer from SpriteRenderer
        LPDIRECT3DVERTEXBUFFER9 pVB = m_spriteRenderer->GetVertexBuffer();
        LPDIRECT3DINDEXBUFFER9 pIB = m_spriteRenderer->GetIndexBuffer();
        LPDIRECT3DVERTEXDECLARATION9 pDecl = m_spriteRenderer->GetVertexDeclaration();
        
        // DEBUG: Check if we have valid resources before ExecuteQueue
        char debugBuf[256];
        sprintf(debugBuf, "[SceneManager::Render] Resources: pVB=%p, pIB=%p, pDecl=%p\n", pVB, pIB, pDecl);
        OutputDebugStringA(debugBuf);
        
        // Execute queue with 32-byte stride (Xbox 360 alignment)
        if (pVB && pIB && pDecl)
        {
            sprintf(debugBuf, "[SceneManager::Render] Calling ExecuteQueue with stride=32\n");
            OutputDebugStringA(debugBuf);
            m_shaderManager->ExecuteQueue(pVB, pIB, pDecl, 32);
        }
        else
        {
            // Fallback: clear queue if resources not available
            m_shaderManager->ClearQueue();
        }
    }
    
    // Step 5: END FRAME - Present to screen (CRITICAL!)
    // Note: EndFrame will be called by GameEngine, not here
    // SceneManager only handles scene rendering and command execution
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

} // namespace Scene
