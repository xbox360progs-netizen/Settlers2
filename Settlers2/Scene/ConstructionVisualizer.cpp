#include "stdafx.h"
#include "ConstructionVisualizer.h"
#include "BuildingPlacement.h"
#include "../World/Map.h"
#include "../World/Flag.h"
#include "../Graphics/SpriteAtlas.h"
#include "../Graphics/TextureRegistry.h"
#include "../World/TileLayer.h"
#include "../Logic/CoordinateSystem.h"

namespace Scene {

    // Construction sprite UV fix — pixel rect (1022,1883,196,139) in 2048x2048 atlas
    const float ConstructionVisualizer::CONSTRUCTION_U0 = 0.199f;
    const float ConstructionVisualizer::CONSTRUCTION_V0 = 0.239f;
    const float ConstructionVisualizer::CONSTRUCTION_U1 = 0.295f;
    const float ConstructionVisualizer::CONSTRUCTION_V1 = 0.307f;
    const uint32_t ConstructionVisualizer::CONSTRUCTION_ATLAS_W = 2048;
    const uint32_t ConstructionVisualizer::CONSTRUCTION_ATLAS_H = 2048;
    const uint32_t ConstructionVisualizer::CONSTRUCTION_PIXEL_X = 1022;
    const uint32_t ConstructionVisualizer::CONSTRUCTION_PIXEL_Y = 1883;
    const uint32_t ConstructionVisualizer::CONSTRUCTION_PIXEL_W = 196;
    const uint32_t ConstructionVisualizer::CONSTRUCTION_PIXEL_H = 139;

    ConstructionVisualizer::ConstructionVisualizer(World::Map* map)
        : m_map(map)
    {
    }

    void ConstructionVisualizer::SetupConstructionSiteTiles(World::Flag* flag, int siteX, int siteY, World::BuildingType buildingType)
    {
        if (!flag || !m_map) return;

        char dbg[256];
        _snprintf(dbg, sizeof(dbg), "[ConstructionVisualizer] SetupConstructionSiteTiles at (%d,%d) for flag at (%d,%d)\n",
            siteX, siteY, flag->pos.x, flag->pos.y);
        OutputDebugStringA(dbg);

        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (!buildingsLayer || siteX < 0 || siteX >= buildingsLayer->GetWidth() || siteY < 0 || siteY >= buildingsLayer->GetHeight()) {
            OutputDebugStringA("[ConstructionVisualizer] SetupConstructionSiteTiles: invalid coordinates\n");
            return;
        }

        int footOffX = 0, footOffY = 0;
        int footW = 1, footH = 1;
        int buildingSpriteIdx = 0;
        std::vector<std::pair<int,int>> footMask;
        {
            const char* namePtr = BuildingPlacementManager::GetBuildingSpriteName(buildingType);
            std::string spriteName = (namePtr && namePtr[0]) ? namePtr : "b_unknown";
            TextureRegistry& reg = TextureRegistry::instance();
            reg.getTextureOrLoad("Buildings");
            std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
            if (buildingsAtlas) {
                uint32_t idx = buildingsAtlas->GetIndex(spriteName.c_str());
                if (idx == 0xFFFFFFFF) {
                    std::string lowerName = spriteName;
                    for (size_t ci = 0; ci < lowerName.size(); ++ci)
                        if (lowerName[ci] >= 'A' && lowerName[ci] <= 'Z')
                            lowerName[ci] = lowerName[ci] - 'A' + 'a';
                    idx = buildingsAtlas->GetIndex(lowerName.c_str());
                }
                if (idx != 0xFFFFFFFF) {
                    buildingSpriteIdx = (int)idx;
                    const SpriteRegion* r = buildingsAtlas->GetRegion(idx);
                    if (r) {
                        footOffX = r->collOffX;
                        footOffY = r->collOffY;
                        footW = (int)r->collWidth;
                        footH = (int)r->collHeight;
                        footMask = r->collMask;
                    }
                }
            }
        }

        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();

        {
            bool is2x2 = (buildingType == World::Stonemason || buildingType == World::Sawmill || buildingType == World::Farm || buildingType == World::Mill);
            if (is2x2 && (footW != 2 || footH != 2)) {
                footW = 2;
                footH = 2;
            }
        }

        auto markTile = [&](int tx, int ty, bool isBaseTile) {
            if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) return;
            World::Tile& bTile = buildingsLayer->GetTile(tx, ty);
            if (bTile.type == World::Tile_None) {
                bTile.type = World::Decoration;
                bTile.walkable = true;
                bTile.buildingType = (int)buildingType;
                if (isBaseTile) {
                    bTile.atlasName = "Buildings";
                    bTile.regionIndex = buildingSpriteIdx;
                    bTile.u0 = CONSTRUCTION_U0; bTile.v0 = CONSTRUCTION_V0;
                    bTile.u1 = CONSTRUCTION_U1; bTile.v1 = CONSTRUCTION_V1;
                } else {
                    bTile.atlasName = "";
                    bTile.regionIndex = -1;
                    bTile.u0 = 0.0f; bTile.v0 = 0.0f;
                    bTile.u1 = 1.0f; bTile.v1 = 1.0f;
                }
            }
        };

        if (!footMask.empty()) {
            for (size_t i = 0; i < footMask.size(); ++i) {
                markTile(siteX + footOffX + footMask[i].first,
                         siteY + footOffY + footMask[i].second,
                         i == 0);
            }
        } else {
            for (int dy = 0; dy < footH; ++dy) {
                for (int dx = 0; dx < footW; ++dx) {
                    markTile(siteX + footOffX + dx, siteY + footOffY + dy,
                             dx == 0 && dy == 0);
                }
            }
        }

        _snprintf(dbg, sizeof(dbg), "[ConstructionVisualizer] SetupConstructionSiteTiles at (%d,%d) type=%d\n",
            siteX, siteY, (int)buildingType);
        OutputDebugStringA(dbg);
    }

    void ConstructionVisualizer::FixConstructionTilesUV()
    {
        if (!m_map) return;

        TextureRegistry& reg = TextureRegistry::instance();
        reg.getTextureOrLoad("Buildings");
        std::tr1::shared_ptr<SpriteAtlas> buildingsAtlas = reg.getAtlas("Buildings");
        if (!buildingsAtlas) return;

        uint32_t cIdx = buildingsAtlas->GetIndex("construction");
        if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("Construction");
        if (cIdx == 0xFFFFFFFF) cIdx = buildingsAtlas->GetIndex("ConstructionSite");
        if (cIdx == 0xFFFFFFFF) {
            OutputDebugStringA("[ConstructionVisualizer] WARNING: construction sprite NOT FOUND in Buildings atlas\n");
            return;
        }

        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (!buildingsLayer) return;

        int fixed = 0;
        for (int y = 0; y < buildingsLayer->GetHeight(); ++y) {
            for (int x = 0; x < buildingsLayer->GetWidth(); ++x) {
                World::Tile& tile = buildingsLayer->GetTile(x, y);
                if (tile.atlasName != "Buildings" || tile.type == World::Tile_None) continue;
                bool isConstr = false;
                const SpriteRegion* rr = buildingsAtlas->GetRegion(static_cast<uint32_t>(tile.regionIndex));
                if (rr) {
                    const std::string& sn = rr->name;
                    isConstr = (sn.find("construction") != std::string::npos ||
                                sn.find("Construction") != std::string::npos);
                }
                if (isConstr ||
                    static_cast<uint32_t>(tile.regionIndex) == cIdx ||
                    (tile.u0 == 0.0f && tile.v0 == 0.0f && tile.u1 == 1.0f && tile.v1 == 1.0f))
                {
                    tile.u0 = CONSTRUCTION_U0;
                    tile.v0 = CONSTRUCTION_V0;
                    tile.u1 = CONSTRUCTION_U1;
                    tile.v1 = CONSTRUCTION_V1;
                    fixed++;
                }
            }
        }
        {
            char dbg[256];
            _snprintf(dbg, sizeof(dbg), "[ConstructionVisualizer] Fixed %d existing construction tile UVs\n", fixed);
            OutputDebugStringA(dbg);
        }
    }

    void ConstructionVisualizer::ClearBuildingFootprint(int startX, int startY, int width, int height)
    {
        if (!m_map) return;
        CoordinateSystem& coords = CoordinateSystem::GetInstance();
        int nodesW = coords.GetNodesWidth();
        int nodesH = coords.GetNodesHeight();
        World::TileLayer* buildingsLayer = m_map->GetLayer(World::Buildings);
        if (!buildingsLayer) return;
        for (int dy = 0; dy < height; ++dy) {
            for (int dx = 0; dx < width; ++dx) {
                int tx = startX + dx;
                int ty = startY + dy;
                if (tx < 0 || tx >= nodesW || ty < 0 || ty >= nodesH) continue;
                World::Tile& t = buildingsLayer->GetTile(tx, ty);
                t.atlasName = "";
                t.type = World::Tile_None;
                t.regionIndex = -1;
                t.walkable = false;
                t.buildingType = -1;
            }
        }
    }

} // namespace Scene
