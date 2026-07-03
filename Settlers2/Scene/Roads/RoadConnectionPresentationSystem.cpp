#include "stdafx.h"
#include "RoadConnectionPresentationSystem.h"
#include "../../World/Map.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void RoadConnectionPresentationSystem::SetMap(World::Map* map)
{
    m_map = map;
}

void RoadConnectionPresentationSystem::BuildRenderFrame(
    std::vector<RenderRoadConnection>& out)
{
    if (!m_map) return;

    World::TileLayer* roadsLayer = m_map->GetLayer(World::Roads);
    if (!roadsLayer) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    int rw = roadsLayer->GetWidth();
    int rh = roadsLayer->GetHeight();

    for (int y = 0; y < rh; ++y) {
        for (int x = 0; x < rw - 1; ++x) {
            const World::Tile& t1 = roadsLayer->GetTile(x, y);
            if (t1.regionIndex < 0 || t1.atlasName != "streets") continue;
            const World::Tile& t2 = roadsLayer->GetTile(x + 1, y);
            if (t2.regionIndex < 0 || t2.atlasName != "streets") continue;

            float wx1, wy1, wx2, wy2;
            coords.NodeTileToWorld(x, y, wx1, wy1);
            coords.NodeTileToWorld(x + 1, y, wx2, wy2);

            RenderRoadConnection seg;
            seg.worldX0 = wx1; seg.worldY0 = wy1;
            seg.worldX1 = wx2; seg.worldY1 = wy2;
            seg.depthLayer = static_cast<uint16_t>(30000 + y * 400);
            out.push_back(seg);
        }
    }
}

} // namespace Scene
