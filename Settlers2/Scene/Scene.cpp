#include "stdafx.h"
#include "Scene.h"
#include "SceneManager.h"

namespace Scene {

SceneManager* SceneBase::GetSceneManager() const
{
    return m_sceneManager;
}

void SceneBase::SetSceneManager(SceneManager* manager)
{
    m_sceneManager = manager;
}

SceneBase::SceneBase(const std::string& name)
    : m_name(name)
    , m_loaded(false)
    , m_hasPendingSwitch(false)
    , m_exitRequested(false)
    , m_sceneManager(nullptr)
{
}

void SceneBase::RequestSceneSwitch(const std::string& sceneName)
{
    m_pendingSceneName = sceneName;
    m_hasPendingSwitch = true;
}

void SceneBase::Initialize(LPDIRECT3DDEVICE9 device, Graphics::SpriteRenderer* spriteRenderer)
{
    (void)device;
    (void)spriteRenderer;
}

SceneBase::~SceneBase()
{
}

void SceneBase::OnEnter()
{
}

void SceneBase::OnExit()
{
}

} // namespace Scene
