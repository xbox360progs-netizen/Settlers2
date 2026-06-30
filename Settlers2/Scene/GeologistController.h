#pragma once
#include "FrameContext.h"

class UIMenu;

namespace World {
    class Map;
}

namespace UI {
    class LocalizationService;
    class NotificationManager;
    class StatusManager;
}

namespace Scene {

    class IGeologistHost {
    public:
        virtual ~IGeologistHost() {}
        virtual void SetGeologistMenuActive(bool active) = 0;
    };

    class GeologistController {
    public:
        GeologistController();
        ~GeologistController();

        void Initialize(World::Map* map,
                        UI::StatusManager* statusManager,
                        IGeologistHost* host,
                        UIMenu* geologistMenu,
                        const UI::LocalizationService* loc,
                        UI::NotificationManager* notificationManager);

        void Update(float dt);
        void FillFrameContext(OverlayFrameState& overlay) const;

        void OnTileAction(int tileX, int tileY);
        void Cancel();

    private:
        int m_state;
        int m_tileX, m_tileY;
        float m_timer;

        World::Map* m_map;
        UI::StatusManager* m_statusManager;
        IGeologistHost* m_host;
        UIMenu* m_geologistMenu;
        const UI::LocalizationService* m_loc;
        UI::NotificationManager* m_notificationManager;
    };

} // namespace Scene
