#include "stdafx.h"
#include "UiController.h"
#include "LocalizationService.h"
#include "../Scene/FrameContext.h"
#include "../World/Map.h"

namespace UI {

    UiController::UiController(Core::EventBus* eventBus, const LocalizationService* loc)
        : m_eventSystem(eventBus)
        , m_notificationManager(loc)
    {
        m_eventSystem.SetNotificationManager(&m_notificationManager);
    }

    UiController::~UiController()
    {
    }

    void UiController::Update(float dt)
    {
        m_eventSystem.Update(dt);
        m_notificationManager.Update(dt);
    }

    void UiController::SetMap(World::Map* map)
    {
        m_eventSystem.SetMap(map);
    }

    void UiController::ShowNotification(const char* title, const char* line1, const char* line2, float duration)
    {
        m_eventSystem.ShowNotification(title, line1, line2, duration);
    }

    void UiController::ShowNotificationAt(int tileX, int tileY, const char* title, const char* line1, const char* line2, float duration)
    {
        m_eventSystem.ShowNotificationAt(tileX, tileY, title, line1, line2, duration);
    }

    void UiController::FillFrameContext(Scene::UiFrameState& state)
    {
        // 1. New notification path (MessageId-based)
        state.notificationCount = 0;
        m_notificationManager.FillFrameContext(state);

        // 2. Legacy notifications from UiEventSystem (append remaining slots)
        int slot = state.notificationCount;
        int legacyCount = m_eventSystem.GetNotificationCount();
        for (int i = 0; i < legacyCount && slot < Scene::UiFrameState::MAX_NOTIFICATIONS; ++i) {
            const World::PopupUiData* src = m_eventSystem.GetNotification(i);
            Scene::UiFrameState::UiNotification& dst = state.notifications[slot];
            if (src && src->isVisible) {
                strncpy_s(dst.title, sizeof(dst.title), src->title, _TRUNCATE);
                strncpy_s(dst.line1, sizeof(dst.line1), src->line1, _TRUNCATE);
                strncpy_s(dst.line2, sizeof(dst.line2), src->line2, _TRUNCATE);
                dst.tileX = src->tileX;
                dst.tileY = src->tileY;
                dst.isActive = true;
                slot++;
            }
        }
        state.notificationCount = slot;
    }

} // namespace UI
