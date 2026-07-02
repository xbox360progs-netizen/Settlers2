#include "stdafx.h"
#include "NotificationPresentationSystem.h"
#include "../FrameContext.h"

namespace Scene {

void NotificationPresentationSystem::BuildRenderFrame(const UiFrameState& uiState, RenderUiFrame& out)
{
    out.notifications.clear();
    if (uiState.notificationCount <= 0) return;

    float boxH = 60.0f;
    for (int i = 0; i < uiState.notificationCount; ++i) {
        const UiFrameState::UiNotification& src = uiState.notifications[i];
        if (!src.isActive) continue;

        RenderNotification dst;
        dst.isActive = true;
        dst.alpha = 1.0f;
        dst.offsetY = (float)i * (boxH + 6.0f);
        strncpy_s(dst.title, sizeof(dst.title), src.title, _TRUNCATE);
        strncpy_s(dst.line1, sizeof(dst.line1), src.line1, _TRUNCATE);
        strncpy_s(dst.line2, sizeof(dst.line2), src.line2, _TRUNCATE);
        out.notifications.push_back(dst);
    }
}

} // namespace Scene
