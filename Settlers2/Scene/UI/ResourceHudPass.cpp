#include "stdafx.h"
#include "ResourceHudPass.h"
#include "RenderResourceHud.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextManager.h"
#include "../../Graphics/RenderLayers.h"

namespace Scene {

ResourceHudPass::ResourceHudPass(TextManager* textManager)
    : m_textManager(textManager)
    , m_textureSlot(0)
{
}

void ResourceHudPass::Execute(const RenderFrame& frame, const RenderContext& context,
                               RenderCommandBuffer& buffer)
{
    const RenderResourceHud& hud = frame.ui.resourceHud;
    if (!hud.loaded || hud.count == 0) return;

    float barX = 100.0f;
    float barY = 6.0f;
    float iconSize = 28.0f;
    float spacing = 60.0f;

    for (int i = 0; i < hud.count; ++i) {
        const RenderResourceHudItem& item = hud.items[i];
        if (item.iconIdx < 0) continue;

        buffer.PushSprite(
            static_cast<int>(barX), static_cast<int>(barY),
            iconSize, iconSize,
            item.u0, item.v0, item.u1, item.v1,
            m_textureSlot, 200, 0xFFFFFFFF,
            0xFFFF, 0xFF, LAYER_FOREGROUND
        );

        if (m_textManager) {
            char buf[32];
            _snprintf(buf, sizeof(buf), "%d", item.stockCount);
            float textX = barX + iconSize + 4.0f;
            float textY = barY + (iconSize - 14.0f) * 0.5f;
            m_textManager->DrawString(buf, textX, textY, 0xFFFFFFFF, 0.07f);
        }

        barX += spacing;
    }
}

} // namespace Scene
