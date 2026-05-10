#pragma once

#include "Scene.h"
#include <map>
#include <string>

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

private:
    std::map<std::string, Scene*> m_scenes;
    Scene* m_currentScene;
};

} // namespace Scene
