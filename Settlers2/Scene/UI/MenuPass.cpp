#include "stdafx.h"
#include "MenuPass.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderContext.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "RenderMenuPanel.h"
#include "../../Graphics/TextManager.h"

namespace Scene {

MenuPass::MenuPass(TextManager* textManager)
    : m_textManager(textManager)
{
}

void MenuPass::Execute(const RenderFrame& frame, const RenderContext& context,
                        RenderCommandBuffer& buffer)
{
    const RenderMenuPanel& panel = frame.ui.menuPanel;

    // ─── Build menu (grid) quads ──────────────────────────────────────
    for (int i = 0; i < panel.buildQuadCount; ++i) {
        const RenderMenuQuad& q = panel.quads[i];
        buffer.PushSprite(static_cast<int16_t>(q.x), static_cast<int16_t>(q.y),
                          static_cast<uint16_t>(q.w), static_cast<uint16_t>(q.h),
                          q.u0, q.v0, q.u1, q.v1,
                          q.textureSlot, q.depthLayer, q.color);
    }

    // Build menu labels
    for (int i = 0; i < panel.buildLabelCount; ++i) {
        const RenderMenuLabel& lbl = panel.labels[i];
        if (lbl.text[0] != '\0' && m_textManager) {
            m_textManager->DrawTextCenteredToScreen(lbl.text, lbl.x, lbl.y, lbl.color, lbl.size);
        }
    }

    // ─── Flag menu (list) quads ───────────────────────────────────────
    for (int i = 0; i < panel.flagQuadCount; ++i) {
        const RenderMenuQuad& q = panel.quads[panel.buildQuadCount + i];
        buffer.PushSprite(static_cast<int16_t>(q.x), static_cast<int16_t>(q.y),
                          static_cast<uint16_t>(q.w), static_cast<uint16_t>(q.h),
                          q.u0, q.v0, q.u1, q.v1,
                          q.textureSlot, q.depthLayer, q.color);
    }

    // Flag menu labels
    for (int i = 0; i < panel.flagLabelCount; ++i) {
        const RenderMenuLabel& lbl = panel.labels[panel.buildLabelCount + i];
        if (lbl.text[0] != '\0' && m_textManager) {
            m_textManager->DrawTextCenteredToScreen(lbl.text, lbl.x, lbl.y, lbl.color, lbl.size);
        }
    }
}

} // namespace Scene
