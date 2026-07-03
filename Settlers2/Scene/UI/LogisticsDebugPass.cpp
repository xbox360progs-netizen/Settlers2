#include "stdafx.h"
#include "LogisticsDebugPass.h"
#include "RenderDebugLabel.h"
#include "../Shared/RenderFrame.h"
#include "../Rendering/RenderCommandBuffer.h"
#include "../../Graphics/TextManager.h"

namespace Scene {

LogisticsDebugPass::LogisticsDebugPass(TextManager* textManager)
    : m_textManager(textManager)
{
}

void LogisticsDebugPass::Execute(
    const RenderFrame& frame, const RenderContext& context,
    RenderCommandBuffer& buffer)
{
    if (frame.debugLabels.empty() || !m_textManager) return;

    for (size_t i = 0; i < frame.debugLabels.size(); ++i) {
        const RenderDebugLabel& l = frame.debugLabels[i];

        if (l.isScreenSpace) {
            m_textManager->DrawTextToScreen(
                l.text, l.worldX, l.worldY,
                l.color, l.scale,
                static_cast<FontID>(l.fontId),
                static_cast<FontStyle>(l.style));
        } else {
            m_textManager->DrawString(
                l.text, l.worldX, l.worldY,
                l.color, l.scale,
                static_cast<FontID>(l.fontId),
                static_cast<FontStyle>(l.style),
                l.depth, l.layer);
        }
    }
}

} // namespace Scene
