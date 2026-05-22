#pragma once

#include <string>
#include <d3d9.h>

// Forward declarations
namespace Graphics { class SpriteRenderer; class RenderQueue; }

namespace Scene {

class SceneManager;

class Scene
{
public:
    Scene(const std::string& name);
    virtual ~Scene();

    const std::string& GetName() const { return m_name; }
    bool IsLoaded() const { return m_loaded; }

    virtual void Load() = 0;
    virtual void Unload() = 0;

    virtual void Update(float deltaTime) = 0;
    virtual void Render(Graphics::RenderQueue* renderQueue) = 0;
    virtual void RenderOverlay() {}

    virtual void OnEnter();
    virtual void OnExit();

    void RequestSceneSwitch(const std::string& sceneName);
    bool HasPendingSceneSwitch() const { return m_hasPendingSwitch; }
    const std::string& GetPendingSceneName() const { return m_pendingSceneName; }
    void ClearPendingSceneSwitch() { m_hasPendingSwitch = false; m_pendingSceneName.clear(); }

    void RequestExit() { m_exitRequested = true; }
    bool IsExitRequested() const { return m_exitRequested; }
    void ClearExitRequest() { m_exitRequested = false; }

    virtual void Initialize(LPDIRECT3DDEVICE9 device, Graphics::SpriteRenderer* spriteRenderer);

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

}
