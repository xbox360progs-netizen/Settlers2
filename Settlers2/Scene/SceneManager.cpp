#include "stdafx.h"
#include "SceneManager.h"
#include <iostream>

namespace Scene {

SceneManager::SceneManager()
    : m_currentScene(NULL)
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
    if (m_currentScene)
    {
        m_currentScene->Render();
    }
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
