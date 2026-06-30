#include "stdafx.h"
#include "MenuCommandDispatcher.h"
#include "SceneManager.h"
#include "LoadingScene.h"
#include "EditorScene.h"

namespace Scene {

    MenuCommandDispatcher::MenuCommandDispatcher(SceneManager* sceneManager)
        : m_sceneManager(sceneManager)
    {
    }

    void MenuCommandDispatcher::Dispatch(const UI::UiAction& action)
    {
        if (!m_sceneManager)
            return;

        Scene* currentScene = m_sceneManager->GetCurrentScene();

        switch (action.command) {

        case UI::UI_CMD_NEW_GAME:
            {
                Scene* loading = m_sceneManager->GetScene("Loading");
                if (loading) {
                    LoadingScene* ls = static_cast<LoadingScene*>(loading);
                    ls->SetTargetScene("Game");
                }
                if (currentScene)
                    currentScene->RequestSceneSwitch("Loading");
            }
            break;

        case UI::UI_CMD_SELECT:
            {
                EditorScene::s_mapGridWidth = action.value;
                EditorScene::s_mapGridHeight = action.value;

                Scene* loading = m_sceneManager->GetScene("Loading");
                if (loading) {
                    LoadingScene* ls = static_cast<LoadingScene*>(loading);
                    ls->SetTargetScene("Editor");
                }
                if (currentScene)
                    currentScene->RequestSceneSwitch("Loading");
            }
            break;

        case UI::UI_CMD_EXIT:
            if (currentScene)
                currentScene->RequestExit();
            break;

        default:
            break;
        }
    }

} // namespace Scene
