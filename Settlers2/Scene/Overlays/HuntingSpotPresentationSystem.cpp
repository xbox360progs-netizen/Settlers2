#include "stdafx.h"
#include "HuntingSpotPresentationSystem.h"
#include "../FrameContext.h"
#include "../PlacementController.h"
#include "../../World/Map.h"
#include "../../World/FlagManager.h"
#include "../../World/Flag.h"
#include "../../World/Components/Building.h"
#include "../../World/ResourceNode.h"
#include "../../Logic/CoordinateSystem.h"
#include "../../Logic/ResourceRegistry.h"

namespace Scene {

void HuntingSpotPresentationSystem::SetManagers(
    World::FlagManager* flagManager, World::Map* map,
    PlacementController* placement)
{
    m_flagManager = flagManager;
    m_map = map;
    m_placement = placement;
}

void HuntingSpotPresentationSystem::BuildRenderFrame(
    const FrameContext& frame,
    std::vector<RenderOverlayMarker>& out)
{
    if (!frame.input.flagMenuActive || !m_flagManager || !m_map || !m_placement)
        return;

    World::Flag* flag = m_flagManager->GetFlagAt(
        m_placement->GetConfirmTargetX(),
        m_placement->GetConfirmTargetY());
    if (!flag || !flag->building || flag->building->type != World::Hunter)
        return;

    Logic::ResourceRegistry* registry = m_map->GetResourceRegistry();
    if (!registry) return;

    const std::vector<Vector2i>& spawners =
        registry->GetWorldResources(World::ResourceType_WildlifeSpawner_Deer);

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    for (size_t si = 0; si < spawners.size(); ++si) {
        const World::ResourceNode& node =
            m_map->GetResourceNode(spawners[si].x, spawners[si].y);
        if (node.type != World::ResourceType_WildlifeSpawner_Deer) continue;

        float wx, wy;
        coords.NodeTileToWorld((float)spawners[si].x, (float)spawners[si].y, wx, wy);

        RenderOverlayMarker marker;
        marker.markerType = OVERLAY_MARKER_HUNTING_SPOT;
        marker.resourceType = static_cast<uint8_t>(World::ResourceType_WildlifeSpawner_Deer);
        marker.amount = static_cast<uint16_t>(node.amount > 0 ? node.amount : 0);
        marker.transform.worldX = wx;
        marker.transform.worldY = wy;
        marker.transform.depthLayer = static_cast<int>(0.99f * 65535.0f);
        out.push_back(marker);
    }
}

} // namespace Scene
