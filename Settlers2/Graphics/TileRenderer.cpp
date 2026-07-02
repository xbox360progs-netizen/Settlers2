#include "stdafx.h"
#include "TileRenderer.h"
#include "../World/TileLayer.h"  // World::LayerType, World::Ground
#include "../Scene/Rendering/RenderCommandBuffer.h"
#include "../Scene/Terrain/RenderTerrainTile.h"
#include "Renderer.h"
#include "TextureRegistry.h"
#include "Texture.h"
#include "SpriteAtlas.h"
#include <cmath>

const float IsoTransform::TILE_WIDTH = 64.0f;
const float IsoTransform::TILE_HEIGHT = 32.0f;
const float IsoTransform::TILE_DEPTH = 32.0f;

void IsoTransform::worldToScreen(int tileX, int tileY, int tileZ,
                                 float& screenX, float& screenY,
                                 float offsetX, float offsetY) {
    screenX = static_cast<float>(tileX - tileY) * (TILE_WIDTH / 2.0f) + offsetX;
    screenY = static_cast<float>(tileX + tileY) * (TILE_HEIGHT / 2.0f) - static_cast<float>(tileZ) * TILE_DEPTH + offsetY;
}

void IsoTransform::screenToWorld(float screenX, float screenY,
                                  int& tileX, int& tileY,
                                  float offsetX, float offsetY) {
    float isoX = screenX - offsetX;
    float isoY = screenY - offsetY;

    tileX = static_cast<int>((isoX / (TILE_WIDTH / 2.0f) + isoY / (TILE_HEIGHT / 2.0f)) / 2.0f);
    tileY = static_cast<int>((isoY / (TILE_HEIGHT / 2.0f) - isoX / (TILE_WIDTH / 2.0f)) / 2.0f);
}

TileRenderer::TileRenderer(::Renderer* renderer, int mapWidth, int mapHeight)
  : m_renderer(renderer), m_mapWidth(mapWidth), m_mapHeight(mapHeight),
    m_mode(0) {}

TileRenderer::~TileRenderer() {}

void TileRenderer::RenderTerrainTiles(const std::vector<Scene::RenderTerrainTile>& tiles,
                                      Scene::RenderCommandBuffer& buffer)
{
    TextureRegistry& reg = TextureRegistry::instance();

    for (size_t i = 0; i < tiles.size(); ++i) {
        const Scene::RenderTerrainTile& rt = tiles[i];

        // Look up atlas → texture slot (set by atlas binding loop in GameRenderer)
        std::string atlasName(rt.atlasName);
        auto it = m_atlasSlots.find(atlasName);
        if (it == m_atlasSlots.end()) continue;
        WORD texSlot = it->second;

        // Layer type determines blend mode already stored in DTO;
        // render layer: ground uses LAYER_TERRAIN, everything else LAYER_WORLD.
        uint8_t renderLayer = (rt.layerType == static_cast<uint8_t>(World::Ground))
            ? LAYER_TERRAIN
            : LAYER_WORLD;

        // screenX/screenY are pre-computed by ProjectionSystem via Camera::WorldToScreen.
        // Use SHADER_WORLD_SCREEN (no VP transform) since coords are in screen space.
        buffer.PushSprite(
            static_cast<int>(rt.screenX + 0.5f),
            static_cast<int>(rt.screenY + 0.5f),
            rt.width, rt.height,
            rt.u0, rt.v0, rt.u1, rt.v1,
            texSlot, rt.depth,
            0xFFFFFFFF,
            SHADER_WORLD_SCREEN, rt.blendMode, renderLayer);
    }
}

void TileRenderer::WorldToScreen(int wx, int wy, int& sx, int& sy) {
    float fx, fy;
    IsoTransform::worldToScreen(wx, wy, 0, fx, fy);
    sx = static_cast<int>(fx);
    sy = static_cast<int>(fy);
}

void TileRenderer::ScreenToWorld(int sx, int sy, int& wx, int& wy) {
    IsoTransform::screenToWorld(static_cast<float>(sx), static_cast<float>(sy), wx, wy);
}

std::pair<int, int> TileRenderer::screenToTileCoords(float screenX, float screenY) const {
    int tileX, tileY;
    IsoTransform::screenToWorld(screenX, screenY, tileX, tileY);
    return std::make_pair(tileX, tileY);
}
