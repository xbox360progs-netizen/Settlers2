#pragma once
#include "../Core/EventBus.h"
#include "../World/UiDefs.h"
#include "UiMessageId.h"
#include <string.h>

namespace World {
    class Map;
}

namespace UI {

    class NotificationManager;

    // UI event system: popup notifications
    // Listens to EventBus events and produces UI notifications.
    // NOT called directly by managers — always via EventBus.Post().
    class UiEventSystem : public Core::EventListener {
    public:
        UiEventSystem(Core::EventBus* eventBus);
        ~UiEventSystem();

        void Update(float dt);
        void SetMap(World::Map* map) { m_map = map; }
        void SetNotificationManager(NotificationManager* mgr) { m_notificationMgr = mgr; }

        // Notifications (legacy raw-string pool — dead after UI4a migration)
        void ShowNotification(const char* title, const char* line1, const char* line2, float duration = 5.0f);
        void ShowNotificationAt(int tileX, int tileY, const char* title, const char* line1, const char* line2, float duration = 5.0f);

        // Access for rendering
        int GetNotificationCount() const { return m_notificationCount; }
        const World::PopupUiData* GetNotification(int index) const;

        // EventListener
        virtual void OnEvent(Core::EventType type, void* data);

    private:
        static const int MAX_NOTIFICATIONS = 4;

        World::PopupUiData m_notifications[MAX_NOTIFICATIONS];
        int m_notificationCount;

        // Notification deduplication (same event within ~2s → coalesce)
        static const int EVENT_CACHE_SIZE = 4;
        struct EventCacheEntry {
            char title[32];
            char line1[32];
            float timer;
            int  count;
        };
        EventCacheEntry m_eventCache[EVENT_CACHE_SIZE];

        Core::EventBus* m_eventBus;
        World::Map* m_map;
        NotificationManager* m_notificationMgr;

        int FindFreeSlot();
        void RemoveNotification(int index);
        bool TryCoalesce(const char* title, const char* line1, float duration);
        void UpdateEventCache(float dt);
        void AddToEventCache(const char* title, const char* line1, float duration);

        void OnConstructionComplete(void* data);
        void OnBuildingPlaced(void* data);
        void OnFlagPlaced(void* data);
        void OnFlagDeleted(void* data);
        void OnResourceDelivered(void* data);
        void OnBuildingProduction(void* data);
    };

} // namespace UI