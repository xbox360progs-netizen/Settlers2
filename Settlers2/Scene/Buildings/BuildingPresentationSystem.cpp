#include "stdafx.h"
#include "BuildingPresentationSystem.h"
#include "../../World/FlagManager.h"
#include "../../World/Flag.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void BuildingPresentationSystem::SetManagers(
    World::FlagManager* flagManager)
{
    m_flagManager = flagManager;
}

void BuildingPresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    CollectFlags(frame.buildings);
    // Note: building sprites are still rendered via Buildings map layer (TileRenderer).
    // When the Buildings layer migrates to RenderFrame, add CollectBuildings here.
}

void BuildingPresentationSystem::CollectFlags(std::vector<RenderBuilding>& out)
{
    if (!m_flagManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    const std::vector<std::pair<int,int> >& pairs = m_flagManager->GetFlagPairs();
    for (size_t i = 0; i < pairs.size(); ++i) {
        int fx = pairs[i].first;
        int fy = pairs[i].second;

        float wx, wy;
        coords.NodeTileToWorld(fx, fy, wx, wy);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + fy * 400;
        rb.visual.kind = 0;              // flag
        rb.visual.buildingType = 0;
        rb.visual.depleted = false;
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

} // namespace Scene
