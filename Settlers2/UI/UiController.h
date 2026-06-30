#pragma once
#include "UiEventSystem.h"
#include "NotificationManager.h"

namespace Scene { struct UiFrameState; }
namespace World { class Map; }

namespace UI {

    class LocalizationService;

    class UiController {
    public:
        UiController(Core::EventBus* eventBus, const LocalizationService* loc);
        ~UiController();

        void Update(float dt);
        void FillFrameContext(Scene::UiFrameState& state);

        // Facade over UiEventSystem
        void SetMap(World::Map* map);
        void ShowNotification(const char* title, const char* line1, const char* line2, float duration = 5.0f);
        void ShowNotificationAt(int tileX, int tileY, const char* title, const char* line1, const char* line2, float duration = 5.0f);

        // New notification path (MessageId-based)
        NotificationManager* GetNotificationManager() { return &m_notificationManager; }

    private:
        UiEventSystem m_eventSystem;
        NotificationManager m_notificationManager;

        // Non-copyable
        UiController(const UiController&);
        UiController& operator=(const UiController&);
    };

} // namespace UI
