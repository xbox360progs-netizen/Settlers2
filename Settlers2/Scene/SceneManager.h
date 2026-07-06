#pragma once

#include "Scene.h"
#include "../Graphics/RenderFrame.h"
#include "../Platform/Lock.h"
#include <map>
#include <string>

// Forward declarations
namespace Graphics { class ShaderManager; class SpriteRenderer; class RenderQueue; }
using Graphics::ShaderManager;
using Graphics::RenderQueue;
class Renderer;
using Graphics::RenderFrame;

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
    void AddScene(SceneBase* scene);
    void RemoveScene(const std::string& name);

    // Переключение сцен
    bool SwitchTo(const std::string& name);
    SceneBase* GetCurrentScene() const { return m_currentScene; }
    const std::string& GetCurrentSceneName() const;

    // Получить сцену по имени
    SceneBase* GetScene(const std::string& name) const;

    // Обновление и рендер текущей сцены
    void Update(float deltaTime);
    void Render();
    void RenderOverlay();

    // Очистка
    void Clear();

    // Queue-based rendering support
    void SetShaderManager(ShaderManager* shaderManager) { m_shaderManager = shaderManager; }
    void SetSpriteRenderer(Graphics::SpriteRenderer* spriteRenderer) { m_spriteRenderer = spriteRenderer; }
    void SetRenderer(Renderer* renderer) { m_renderer = renderer; }
    Graphics::SpriteRenderer* GetSpriteRenderer() const { return m_spriteRenderer; }
    Renderer* GetRenderer() const { return m_renderer; }
    void SetRenderFrame(RenderFrame* frame) { m_renderFrame = frame; }
    RenderFrame* GetRenderFrame() const { return m_renderFrame; }
    void SetRenderQueue(RenderQueue* queue) { m_renderQueue = queue; }
    RenderQueue* GetRenderQueue() const { return m_renderQueue; }

    // Thread barrier for scene readiness (prevents Core 1 render thread from accessing unloaded resources)
    bool IsSceneReady() const { return m_isSceneReady; }
    void SetSceneReady(bool ready) { m_isSceneReady = ready; }

    // Additional barrier: graphics resources specifically (textures, buffers)
    bool IsGraphicsReady() const { return m_bSceneGraphicsReady; }
    void SetGraphicsReady(bool ready) { m_bSceneGraphicsReady = ready; }

    // Thread-safe scene access
    void Lock() { m_lock.Acquire(); }
    void Unlock() { m_lock.Release(); }

    // Frame rendering flag management
    void ResetFrameRendered();

private:
    std::map<std::string, SceneBase*> m_scenes;
    SceneBase* volatile m_currentScene;  // volatile for Xenon cache coherency
    ShaderManager* m_shaderManager;
    Graphics::SpriteRenderer* m_spriteRenderer;
    Renderer* m_renderer;
    RenderFrame* m_renderFrame;
    RenderQueue* m_renderQueue;

    // Thread barrier for scene readiness
    volatile bool m_isSceneReady;
    volatile bool m_bSceneGraphicsReady;

    // Frame rendering flag to prevent duplicate rendering
    bool m_frameRendered;

    static SceneManager* s_pInstance;

    // Critical section for thread-safe scene switching
    Platform::Lock m_lock;
};

} // namespace Scene
