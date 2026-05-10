#include "stdafx.h"
#include "Scene.h"
#include "SceneManager.h"

namespace Scene {

SceneManager* Scene::GetSceneManager() const
{
    return m_sceneManager;
}

void Scene::SetSceneManager(SceneManager* manager)
{
    m_sceneManager = manager;
}

Scene::Scene(const std::string& name)
    : m_name(name)
    , m_loaded(false)
    , m_hasPendingSwitch(false)
    , m_exitRequested(false)
    , m_sceneManager(nullptr)
{
}

void Scene::RequestSceneSwitch(const std::string& sceneName)
{
    m_pendingSceneName = sceneName;
    m_hasPendingSwitch = true;
}

void Scene::Initialize(LPDIRECT3DDEVICE9 device, SpriteRenderer* spriteRenderer)
{
    (void)device;
    (void)spriteRenderer;
    // Базовая реализация - ничего не делает
    // Производные классы могут переопределить
}

Scene::~Scene()
{
}

void Scene::OnEnter()
{
}

void Scene::OnExit()
{
}

} // namespace Scene
