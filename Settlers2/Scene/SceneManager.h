#pragma once

#include "Scene.h"
#include <map>
#include <string>
#include <xtl.h>

// Forward declaration
class ShaderManager;
class SpriteRenderer;

// Xbox 360 async command buffer forward declarations
#ifdef _XBOX
struct IDirect3DAsyncCommandBufferCall9;
struct IDirect3DCommandBuffer9;
#endif

namespace Scene {

class SceneManager
{
public:
    static SceneManager* Instance() {
        return s_pInstance;
    }

    SceneManager();
    ~SceneManager();

    // Управление сценами
    void AddScene(Scene* scene);
    void RemoveScene(const std::string& name);

    // Переключение сцен
    bool SwitchTo(const std::string& name);
    Scene* GetCurrentScene() const { return m_currentScene; }
    const std::string& GetCurrentSceneName() const;

    // Получить сцену по имени
    Scene* GetScene(const std::string& name) const;

    // Обновление и рендер текущей сцены
    void Update(float deltaTime);
    void Render();

    // Очистка
    void Clear();

    // Queue-based rendering support
    void SetShaderManager(ShaderManager* shaderManager) { m_shaderManager = shaderManager; }
    void SetSpriteRenderer(SpriteRenderer* spriteRenderer) { m_spriteRenderer = spriteRenderer; }
    SpriteRenderer* GetSpriteRenderer() const { return m_spriteRenderer; }

    // Xbox 360 async command buffer support
#ifdef _XBOX
    void InitializeAsyncCommandBuffer(LPDIRECT3DDEVICE9 pDevice);
    IDirect3DCommandBuffer9* GetSpriteCommandBuffer() const { return m_pCommandBuffer; }
    IDirect3DAsyncCommandBufferCall9* GetAsyncCall() const { return m_pAsyncCall; }
    IDirect3DCommandBuffer9* GetRecordCommandBuffer() const { return m_pRecordCommandBuffer; }
    void SetRecordCommandBuffer(IDirect3DCommandBuffer9* buf) { m_pRecordCommandBuffer = buf; }
#endif

    // Thread barrier for scene readiness (prevents Core 1 render thread from accessing unloaded resources)
    bool IsSceneReady() const { return m_isSceneReady; }
    void SetSceneReady(bool ready) { m_isSceneReady = ready; }

    // Additional barrier: graphics resources specifically (textures, buffers)
    bool IsGraphicsReady() const { return m_bSceneGraphicsReady; }
    void SetGraphicsReady(bool ready) { m_bSceneGraphicsReady = ready; }

    // Thread-safe scene access
    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

    // Frame rendering flag management
    void ResetFrameRendered();

private:
    std::map<std::string, Scene*> m_scenes;
    Scene* volatile m_currentScene;  // volatile for Xenon cache coherency
    ShaderManager* m_shaderManager;
    SpriteRenderer* m_spriteRenderer;

#ifdef _XBOX
    IDirect3DAsyncCommandBufferCall9* m_pAsyncCall;
    IDirect3DCommandBuffer9* m_pCommandBuffer;
    IDirect3DCommandBuffer9* m_pRecordCommandBuffer;
#endif

    // Thread barrier for scene readiness
    volatile bool m_isSceneReady;
    volatile bool m_bSceneGraphicsReady;

    // Frame rendering flag to prevent duplicate rendering
    bool m_frameRendered;

    static SceneManager* s_pInstance;

    // Critical section for thread-safe scene switching
    CRITICAL_SECTION m_cs;
};

} // namespace Scene
