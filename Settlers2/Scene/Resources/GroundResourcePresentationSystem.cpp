#include "GroundResourcePresentationSystem.h"
#include "RenderGroundResource.h"
#include "../../World/Map.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

GroundResourcePresentationSystem::GroundResourcePresentationSystem()
    : m_map(NULL)
{
}

void GroundResourcePresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    if (!m_map) return;

    frame.groundResources.clear();

    int n = m_map->GetGroundResourceCount();
    if (n <= 0) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int gi = 0; gi < n; ++gi) {
        World::GroundResource* gr = m_map->GetGroundResource(gi);
        if (!gr) continue;

        float wx, wy;
        coords.NodeTileToWorld(gr->pos.x, gr->pos.y, wx, wy);

        RenderGroundResource rgr;
        rgr.transform.worldX = wx;
        rgr.transform.worldY = wy;
        rgr.transform.depthLayer = 0; // set by Projection
        rgr.resourceType = gr->type;
        rgr.amount = gr->amount;
        rgr.visualOnly = gr->visualOnly;

        frame.groundResources.push_back(rgr);
    }
}

} // namespace Scene
