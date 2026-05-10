#pragma once

#include <string>
#include <d3d9.h>

// Forward declarations
class SpriteRenderer;

namespace Scene {

class SceneManager;  // Forward declare inside Scene namespace

class Scene
{
public:
    Scene(const std::string& name);
    virtual ~Scene();

    const std::string& GetName() const { return m_name; }
    bool IsLoaded() const { return m_loaded; }

    // Жизненный цикл сцены
    virtual void Load() = 0;
    virtual void Unload() = 0;

    // Обновление и рендер
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;

    // Сцена стала активной / неактивной
    virtual void OnEnter();
    virtual void OnExit();

    // Запрос на переключение сцены
    void RequestSceneSwitch(const std::string& sceneName);
    bool HasPendingSceneSwitch() const { return m_hasPendingSwitch; }
    const std::string& GetPendingSceneName() const { return m_pendingSceneName; }
    void ClearPendingSceneSwitch() { m_hasPendingSwitch = false; m_pendingSceneName.clear(); }

    // Запрос на выход из приложения
    void RequestExit() { m_exitRequested = true; }
    bool IsExitRequested() const { return m_exitRequested; }
    void ClearExitRequest() { m_exitRequested = false; }

    // Инициализация с устройством (вызвать перед использованием)
    virtual void Initialize(LPDIRECT3DDEVICE9 device, class SpriteRenderer* spriteRenderer);

    // SceneManager access
    SceneManager* GetSceneManager() const;
    void SetSceneManager(SceneManager* manager);

protected:
    std::string m_name;
    bool m_loaded;
    bool m_hasPendingSwitch;
    std::string m_pendingSceneName;
    bool m_exitRequested;
    SceneManager* m_sceneManager;
};

} // namespace Scene
