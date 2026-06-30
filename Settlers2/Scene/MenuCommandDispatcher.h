#pragma once
#include "../UI/ICommandDispatcher.h"

namespace Scene {

    class SceneManager;

    class MenuCommandDispatcher : public UI::ICommandDispatcher {
    public:
        explicit MenuCommandDispatcher(SceneManager* sceneManager);

        void Dispatch(const UI::UiAction& action);

    private:
        SceneManager* m_sceneManager;
    };

} // namespace Scene
