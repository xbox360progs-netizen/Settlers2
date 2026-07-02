#include "stdafx.h"
#include "PlacementPreviewPresentationSystem.h"
#include "../PlacementController.h"
#include "../BuildingPlacement.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void PlacementPreviewPresentationSystem::BuildRenderFrame(
    const FrameContext& frame,
    std::vector<RenderPlacementPreview>& out)
{
    if (!m_placement) return;
    if (m_placement->GetState() != PLACESTATE_PLACE_FLAG) return;
    if (m_placement->IsIdle()) return;

    PlacementData pd = m_placement->GetPlacementData(frame.input.cursorTileX, frame.input.cursorTileY);
    if (!pd.spriteRegion) return;

    // Pre-compute world position.
    float wx, wy;
    CoordinateSystem::GetInstance().NodeTileToWorld(pd.buildX, pd.buildY, wx, wy);

    RenderPlacementPreview rp;
    rp.transform.worldX = wx;
    rp.transform.worldY = wy;
    rp.transform.depthLayer = static_cast<int>(0.98f * 65535.0f);
    rp.visual.type    = static_cast<uint8_t>(m_placement->GetSelectedBuilding());
    rp.visual.allowed = pd.valid;

    out.push_back(rp);
}

} // namespace Scene
