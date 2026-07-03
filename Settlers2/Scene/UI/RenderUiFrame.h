#pragma once
#include <vector>
#include "RenderConfirmationMenu.h"
#include "RenderNotification.h"
#include "RenderTownHallPanel.h"
#include "RenderResourceHud.h"
#include "RenderBanner.h"
#include "RenderMenuPanel.h"

namespace Scene {

struct RenderUiFrame {
    RenderConfirmationMenu confirmation;
    std::vector<RenderNotification> notifications;
    RenderTownHallPanel townHallPanel;
    RenderResourceHud resourceHud;
    RenderBanner      banner;
    RenderMenuPanel   menuPanel;

    void Clear() {
        notifications.clear();
        menuPanel = RenderMenuPanel();  // reset counts
    }
};

} // namespace Scene
