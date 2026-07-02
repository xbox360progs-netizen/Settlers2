#include "stdafx.h"
#include "TerrainPresentationSystem.h"
#include "RenderTerrainTile.h"
#include "../Shared/RenderFrame.h"
#include "../../World/Map.h"
#include "../../World/TileLayer.h"
#include "../../World/Tile.h"
#include "../../Graphics/SpriteAtlas.h"
#include "../../Graphics/TextureRegistry.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

TerrainPresentationSystem::TerrainPresentationSystem()
    : m_map(NULL)
    , m_mapWidth(0)
    , m_mapHeight(0)
{
}

TerrainPresentationSystem::~TerrainPresentationSystem()
{
}

void TerrainPresentationSystem::SetMap(World::Map* map, int mapWidth, int mapHeight)
{
    m_map = map;
    m_mapWidth = mapWidth;
    m_mapHeight = mapHeight;
}

void TerrainPresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    if (!m_map) return;

    CoordinateSystem::GetInstance().Initialize(m_mapWidth, m_mapHeight);

    std::vector<RenderTerrainTile>& tiles = frame.terrain;
    tiles.clear();

    // Same layer order as TileRenderer::RenderMap():
    // Roads → Ground → Objects → Buildings
    ResolveLayer(static_cast<int>(World::Roads), tiles);
    ResolveLayer(static_cast<int>(World::Ground), tiles);
    ResolveLayer(static_cast<int>(World::Objects), tiles);
    ResolveLayer(static_cast<int>(World::Buildings), tiles);
}

void TerrainPresentationSystem::ResolveLayer(int layerType, std::vector<RenderTerrainTile>& outTiles)
{
    World::TileLayer* layer = m_map->GetLayer(static_cast<World::LayerType>(layerType));
    if (!layer) return;

    int width = layer->GetWidth();
    int height = layer->GetHeight();
    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    TextureRegistry& reg = TextureRegistry::instance();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const World::Tile& tile = layer->GetTile(x, y);
            if (tile.type == World::Tile_None) continue;
            if (tile.atlasName.empty()) continue;

            RenderTerrainTile rt;
            memset(&rt, 0, sizeof(rt));
            rt.layerType = static_cast<uint8_t>(layerType);

            // Copy atlas name (fixed-size buffer)
            size_t nameLen = tile.atlasName.length();
            if (nameLen > 31) nameLen = 31;
            memcpy(rt.atlasName, tile.atlasName.c_str(), nameLen);
            rt.atlasName[nameLen] = '\0';

            // Compute world coords, sprite dimensions, UVs — matches
            // TileRenderer::RenderTile() logic.
            float wx, wy;
            float spriteW, spriteH, pivotX, pivotY;
            float useU0 = tile.u0, useV0 = tile.v0;
            float useU1 = tile.u1, useV1 = tile.v1;

            if (layerType == World::Ground) {
                coords.GroundTileToWorld(x, y, wx, wy);
                spriteW = 238.0f;
                spriteH = 148.0f;
                pivotX = spriteW * 0.5f;
                pivotY = spriteH * 0.5f;

                std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(tile.atlasName);
                if (tile.regionIndex >= 0 && atlas) {
                    const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
                    if (region) {
                        spriteW = static_cast<float>(region->width);
                        spriteH = static_cast<float>(region->height);
                        pivotX = region->pivotX;
                        pivotY = region->pivotY;
                    }
                }
                wx += 119.0f;
                wy += 74.0f;
                rt.blendMode = 1;
                rt.depth = static_cast<uint16_t>(0.95f * 65535.0f);
            } else {
                coords.NodeTileToWorld(x, y, wx, wy);
                spriteW = 119.0f;
                spriteH = 72.0f;
                pivotX = 0.0f;
                pivotY = 0.0f;

                std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(tile.atlasName);
                if (tile.regionIndex >= 0 && atlas) {
                    const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
                    if (region) {
                        spriteW = static_cast<float>(region->width);
                        spriteH = static_cast<float>(region->height);
                        pivotX = region->pivotX;
                        pivotY = region->pivotY;
                    }
                }
                rt.blendMode = 0;
                rt.depth = static_cast<uint16_t>(30010 + y * 400);
            }

            rt.worldX = wx - pivotX;
            rt.worldY = wy - pivotY;
            rt.screenX = 0.0f;
            rt.screenY = 0.0f;
            rt.width = spriteW;
            rt.height = spriteH;
            rt.u0 = useU0; rt.v0 = useV0;
            rt.u1 = useU1; rt.v1 = useV1;

            outTiles.push_back(rt);
        }
    }
}

}
