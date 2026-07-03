#include "stdafx.h"
#include "NotificationPass.h"
#include "RenderUiFrame.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextManager.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

NotificationPass::NotificationPass(TextManager* textManager)
    : m_slot(0)
    , m_textManager(textManager)
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

        // Notification text (pre-resolved in NotificationPresentationSystem).
        if (m_textManager) {
            float textX = startX + 10.0f;
            m_textManager->DrawString(n.title, textX, yPos + 4.0f,
                D3DCOLOR_ARGB(255, 255, 200, 80), 0.07f);
            m_textManager->DrawString(n.line1, textX, yPos + 22.0f,
                D3DCOLOR_ARGB(255, 220, 220, 220), 0.06f);
            if (n.line2[0] != '\0') {
                m_textManager->DrawString(n.line2, textX, yPos + 38.0f,
                    D3DCOLOR_ARGB(255, 180, 180, 180), 0.055f);
            }
        }
    }
}

} // namespace Scene
