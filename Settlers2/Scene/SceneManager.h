#pragma once

#include "Scene.h"
#include <map>
#include <string>

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

    // Xbox 360 async command buffer support
#ifdef _XBOX
    void InitializeAsyncCommandBuffer(LPDIRECT3DDEVICE9 pDevice);
    IDirect3DCommandBuffer9* GetSpriteCommandBuffer() const { return m_pCommandBuffer; }
    IDirect3DAsyncCommandBufferCall9* GetAsyncCall() const { return m_pAsyncCall; }
#endif

private:
    std::map<std::string, Scene*> m_scenes;
    Scene* m_currentScene;
    ShaderManager* m_shaderManager;
    SpriteRenderer* m_spriteRenderer;

#ifdef _XBOX
    IDirect3DAsyncCommandBufferCall9* m_pAsyncCall;
    IDirect3DCommandBuffer9* m_pCommandBuffer;
#endif
};

} // namespace Scene
