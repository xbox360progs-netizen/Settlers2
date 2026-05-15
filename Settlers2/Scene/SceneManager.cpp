#include "stdafx.h"
#include "SceneManager.h"
#include "../Graphics/ShaderManager.h"
#include "../Graphics/SpriteRenderer.h"
#include "../Graphics/RenderTypes.h"
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
#ifdef _XBOX
    , m_pAsyncCall(NULL)
    , m_pCommandBuffer(NULL)
    , m_pRecordCommandBuffer(NULL)
#endif
    , m_isSceneReady(false)
    , m_bSceneGraphicsReady(false)
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

    // Enter new scene
    m_currentScene = it->second;
    std::cout << "[SceneManager] Entering new scene: " << m_currentScene->GetName() << std::endl;

    // Load if not loaded
    if (!m_currentScene->IsLoaded())
    {
        std::cout << "[SceneManager] Loading scene: " << m_currentScene->GetName() << std::endl;
        m_currentScene->Load();
    }

    m_currentScene->OnEnter();
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
    if (!m_currentScene)
    {
        return;
    }

    // Check if scene is ready for rendering (prevents race conditions during initialization)
    if (!m_isSceneReady || !m_bSceneGraphicsReady)
    {
        OutputDebugStringA("[SceneManager::Render] Scene not ready yet, skipping render\n");
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

    char dbg[512];
    sprintf(dbg, "[SM::Render] ENTRY - m_currentScene=0x%08X\n", m_currentScene);
    OutputDebugStringA(dbg);
    
    // THREAD SAFETY: Lock BEFORE accessing m_currentScene
    EnterCriticalSection(&m_cs);
    
    sprintf(dbg, "[SM::Render] AFTER LOCK - m_currentScene=0x%08X\n", m_currentScene);
    OutputDebugStringA(dbg);
    
    // GUARD: If pointer is null
    if (m_currentScene == nullptr) {
        OutputDebugStringA("[SM::Render] ERROR: m_currentScene is NULL!\n");
        LeaveCriticalSection(&m_cs);
        return;
    }
    
    // Dump vtable
    void** vtable = *(void***)m_currentScene;
    sprintf(dbg, "[SM::Render] Vtable=0x%08X\n", vtable);
    OutputDebugStringA(dbg);
    
    if (vtable == nullptr) {
        OutputDebugStringA("[FATAL] Scene Vtable is NULL!\n");
        LeaveCriticalSection(&m_cs);
        return;
    }
    
    OutputDebugStringA("[SM::Render] About to call m_currentScene->Render()...\n");
    m_currentScene->Render();
    OutputDebugStringA("[SM::Render] m_currentScene->Render() RETURNED!\n");
    
    LeaveCriticalSection(&m_cs);
    OutputDebugStringA("[SM::Render] EXIT\n");

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
        char debugBuf[512];
        sprintf(debugBuf, "[SM::Render] Resources: pVB=%p, pIB=%p, pDecl=%p\n", pVB, pIB, pDecl);
        OutputDebugStringA(debugBuf);
        
        // Execute queue with 32-byte stride (Xbox 360 alignment)
        if (pVB && pIB && pDecl)
        {
            OutputDebugStringA("[SM::Render] Calling ExecuteQueue...\n");
            m_shaderManager->ExecuteQueue(pVB, pIB, pDecl, 32);
            OutputDebugStringA("[SM::Render] ExecuteQueue RETURNED!\n");
        }
        else
        {
            OutputDebugStringA("[SM::Render] NULL resources - clearing queue\n");
            m_shaderManager->ClearQueue();
        }
    }
    
    OutputDebugStringA("[SM::Render] ALL DONE - exiting Render()\n");
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
