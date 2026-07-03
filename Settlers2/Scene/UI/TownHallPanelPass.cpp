#include "stdafx.h"
#include "TownHallPanelPass.h"
#include "RenderTownHallPanel.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextManager.h"
#include "../../Graphics/RenderLayers.h"
#include <cstdio>

namespace Scene {

const char* TownHallPanelPass::kStockNames[6] = {
    "Wood:", "Planks:", "Stone:", "Fish:", "Meat:", "Coal:"
};

const int TownHallPanelPass::kStockTypes[6] = {
    0, 1, 2, 3, 4, 5  // indices into stockValues[]
};

TownHallPanelPass::TownHallPanelPass(TextManager* textManager)
    : m_textureSlot(0)
    , m_textManager(textManager)
{
}

void TownHallPanelPass::Execute(
    const RenderFrame& frame, const RenderContext& context,
    RenderCommandBuffer& buffer)
{
    const RenderTownHallPanel& panel = frame.ui.townHallPanel;
    if (!panel.visible) return;

    // Panel background sprite.
    buffer.PushSprite(
        static_cast<int>(panel.panelX + 0.5f),
        static_cast<int>(panel.panelY + 0.5f),
        panel.panelW, panel.panelH,
        panel.panelU0, panel.panelV0,
        panel.panelU1, panel.panelV1,
        m_textureSlot, 10, 0xFFFFFFFF,
        0xFFFF, 0xFF, LAYER_UI);

    // Stock text.
    if (m_textManager) {
        float tx = panel.panelX + 40.0f;
        float ty = panel.panelY + 30.0f;
        float lineH = 28.0f;

        for (int i = 0; i < RenderTownHallPanel::STOCK_COUNT; ++i) {
            char buf[64];
            _snprintf(buf, sizeof(buf), "%s %d",
                kStockNames[i], panel.stockValues[i]);
            m_textManager->DrawString(buf, tx, ty,
                0xFFFFFFFF, 0.08f);
            ty += lineH;
        }
    }
}

} // namespace Scene
