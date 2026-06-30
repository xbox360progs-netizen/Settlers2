#pragma once

#include "../World/Components/Building.h"

namespace Scene {

// ─── Input state snapshot (filled by InputController::FillFrameContext) ──
struct InputFrameState {
    int   cursorTileX;
    int   cursorTileY;
    bool  gamepadActive;
    int   gamepadCursorX;
    int   gamepadCursorY;
    bool  menuActive;
    bool  roadMenuActive;
    bool  flagMenuActive;
    bool  geologistMenuActive;
    bool  townHallPanelOpen;
    bool  cursorOnTownHall;
    char  statusText[64];
    bool  logisticsDebug;
};

// ─── UI state snapshot (filled by UiController::FillFrameContext) ─────────
struct UiFrameState {
    static const int MAX_NOTIFICATIONS = 4;
    struct UiNotification {
        char title[32];
        char line1[32];
        char line2[32];
        int  tileX;
        int  tileY;
        bool isActive;
    };
    UiNotification notifications[MAX_NOTIFICATIONS];
    int            notificationCount;
};

// ─── Game-state overlay (filled by GameScene per frame) ──────────────────
struct OverlayFrameState {
    int   geologistState;
    int   geologistTileX;
    int   geologistTileY;

    static const int GEOLOGIST_NONE    = 0;
    static const int GEOLOGIST_CONFIRM = 1;
    static const int GEOLOGIST_WORKING = 2;

    float bannerSlideX;
    float bannerW, bannerH, bannerU0, bannerV0, bannerU1, bannerV1;
    bool  bannerLoaded;

    int   townHallPanelBgIdx;
    float townHallPanelU0, townHallPanelV0, townHallPanelU1, townHallPanelV1;
    float townHallPanelW, townHallPanelH;

    struct ResourceHudItem {
        World::ResourceType type;
        const char*         iconName;
        int                 iconIdx;
        int                 showOrder;
    };
    static const int RESOURCE_HUD_COUNT = 11;
    ResourceHudItem resourceHud[RESOURCE_HUD_COUNT];
    bool             resourceHudLoaded;

    OverlayFrameState();
};

// ─── Per-frame render context (POD, no heap) ─────────────────────────────
struct FrameContext {
    InputFrameState   input;
    UiFrameState      ui;
    OverlayFrameState overlay;
};

} // namespace Scene
