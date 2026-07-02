#pragma once
#include <vector>
#include "RenderConfirmationMenu.h"
#include "RenderNotification.h"

namespace Scene {

struct RenderUiFrame {
    RenderConfirmationMenu confirmation;
    std::vector<RenderNotification> notifications;

    void Clear() {
        notifications.clear();
    }
};

} // namespace Scene
