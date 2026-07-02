#include "stdafx.h"
#include "NotificationPass.h"
#include "RenderUiFrame.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

NotificationPass::NotificationPass()
    : m_slot(0)
{
}

void NotificationPass::Execute(
    const RenderFrame& frame, const RenderContext& context,
    RenderCommandBuffer& buffer)
{
    const std::vector<RenderNotification>& notifications = frame.ui.notifications;
    if (notifications.empty()) return;

    float startX = 1280.0f - 280.0f;
    float startY = 20.0f;
    float boxW = 260.0f;
    float boxH = 60.0f;

    for (size_t i = 0; i < notifications.size(); ++i) {
        const RenderNotification& n = notifications[i];
        if (!n.isActive) continue;

        float yPos = startY + n.offsetY;

        buffer.PushSprite(
            startX, yPos, boxW, boxH,
            0.0f, 0.0f, 1.0f, 1.0f,
            m_slot,
            static_cast<WORD>(0.95f * 65535.0f),
            0xC8141428,
            0xFFFF, 0xFF, LAYER_UI);
    }
}

} // namespace Scene
