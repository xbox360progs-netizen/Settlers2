#include "stdafx.h"
#include "CursorPresentationSystem.h"
#include "../FrameContext.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void CursorPresentationSystem::BuildRenderFrame(const FrameContext& frame, RenderCursor& outCursor)
{
    outCursor.valid = false;

    // Suppressed by placement preview (Stage 7B) or geologist overlay.
    if (m_suppressed) return;

    // Only show cursor when no menu is active (matches legacy RenderCursor guard).
    if (frame.input.menuActive ||
        frame.input.roadMenuActive ||
        frame.input.flagMenuActive ||
        frame.input.geologistMenuActive) {
        return;
    }

    float wx, wy;
    CoordinateSystem::GetInstance().NodeTileToWorld(
        frame.input.cursorTileX, frame.input.cursorTileY, wx, wy);

    outCursor.worldX = wx;
    outCursor.worldY = wy;
    outCursor.valid = true;
}

} // namespace Scene
