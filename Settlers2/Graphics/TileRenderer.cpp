#include "stdafx.h"
#include "TileRenderer.h"
#include "Renderer.h"
#include "TextureRegistry.h"
#include "Texture.h"
#include "SpriteAtlas.h"
#include "RenderLayers.h"
#include "../Logic/CoordinateSystem.h"
#include <cmath>
#include <cstdio>

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
  : m_renderer(renderer), m_map(nullptr), m_renderQueue(nullptr), m_mapWidth(mapWidth), m_mapHeight(mapHeight),
    m_mode(0) {}

TileRenderer::~TileRenderer() {}

void TileRenderer::RenderMap() {
    if (!m_map) return;

    CoordinateSystem::GetInstance().Initialize(m_mapWidth, m_mapHeight);

    // Render only game-relevant layers (skip Placement debug, Resources, etc.)
    RenderTileLayer(World::Roads, World::Roads);
    RenderTileLayer(World::Ground, World::Ground);
    RenderTileLayer(World::Objects, World::Objects);
    RenderTileLayer(World::Buildings, World::Buildings);
}

void TileRenderer::RenderTileLayer(World::LayerType layer, int layerOffset) {
    if (!m_map) return;

    World::TileLayer* tileLayer = m_map->GetLayer(layer);
    if (!tileLayer) return;

    int width = tileLayer->GetWidth();
    int height = tileLayer->GetHeight();
    int nonNoneCount = 0;
    int submittedCount = 0;
    const char* layerNames[] = {
        "Roads", "Nodes", "Placement", "Resources",
        "Ground", "Objects", "Overlay", "Buildings"
    };
    const char* layerName = (layer >= 0 && layer < 8) ? layerNames[layer] : "?";

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const World::Tile& tile = tileLayer->GetTile(x, y);
            if (tile.type != World::Tile_None) {
                nonNoneCount++;
                if (!tile.atlasName.empty()) {
                    submittedCount++;
                }
            }
        }
    }
    char buf[256];
    _snprintf(buf, sizeof(buf), "[TileRenderer] Layer %d (%s): %dx%d, non-None=%d, submitted=%d\n",
              (int)layer, layerName, width, height, nonNoneCount, submittedCount);
    OutputDebugStringA(buf);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const World::Tile& tile = tileLayer->GetTile(x, y);
            if (tile.type != World::Tile_None) {
                RenderTile(x, y, tile.type, layerOffset);
            }
        }
    }
}

void TileRenderer::RenderTile(int tileX, int tileY, World::TileType type, int layerOffset) {
    (void)type;
    if (!m_map || !m_renderQueue) return;

    World::LayerType layerType = static_cast<World::LayerType>(layerOffset);
    World::TileLayer* layer = m_map->GetLayer(layerType);
    if (!layer) return;
    const World::Tile& tile = layer->GetTile(tileX, tileY);

    // Look up texture slot for this tile's atlas
    if (tile.atlasName.empty()) return;
    auto it = m_atlasSlots.find(tile.atlasName);
    if (it == m_atlasSlots.end()) return;
    WORD texSlot = it->second;

    // Get the texture
    TextureRegistry& reg = TextureRegistry::instance();
    LPDIRECT3DTEXTURE9 tex = NULL;
    std::tr1::shared_ptr<SpriteAtlas> atlas = reg.getAtlas(tile.atlasName);
    if (atlas) {
        tex = atlas->GetTexture();
    }
    if (!tex) {
        tex = reg.getTexture(tile.atlasName);
    }
    if (!tex) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    float wx, wy;
    float spriteW, spriteH, pivotX, pivotY;
    float useU0 = tile.u0, useV0 = tile.v0, useU1 = tile.u1, useV1 = tile.v1;
    BYTE blendMode;
    BYTE renderLayer;
    WORD depth;

    if (layerType == World::Ground) {
        // Ground tiles: simple grid
        coords.GroundTileToWorld(tileX, tileY, wx, wy);
        spriteW = 238.0f;
        spriteH = 148.0f;
        pivotX = spriteW * 0.5f;
        pivotY = spriteH * 0.5f;

        if (tile.regionIndex >= 0 && atlas) {
            const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
            if (region) {
                spriteW = (float)region->width;
                spriteH = (float)region->height;
                pivotX = region->pivotX;
                pivotY = region->pivotY;
            }
        }
        wx += 119.0f;
        wy += 74.0f;
        blendMode = 1;
        renderLayer = LAYER_TERRAIN;
        depth = static_cast<WORD>(0.95f * 65535.0f);
    } else {
        // Node-based tiles (objects, buildings, roads): staggered grid
        coords.NodeTileToWorld(tileX, tileY, wx, wy);
        spriteW = 119.0f;
        spriteH = 72.0f;
        pivotX = 0.0f;
        pivotY = 0.0f;

        if (tile.regionIndex >= 0 && atlas) {
            const SpriteRegion* region = atlas->GetRegion(tile.regionIndex);
            if (region) {
                spriteW = (float)region->width;
                spriteH = (float)region->height;
                pivotX = region->pivotX;
                pivotY = region->pivotY;
            }
        }
        blendMode = 0;
        renderLayer = LAYER_WORLD;
        depth = static_cast<WORD>(30010 + tileY * 400);
    }

    // Submit in world space — camera view-projection matrix transforms to screen
    float renderX = wx - pivotX;
    float renderY = wy - pivotY;

    submitTile(renderX, renderY, spriteW, spriteH,
               tex, texSlot,
               useU0, useV0, useU1, useV1,
               SHADER_TERRAIN, blendMode, renderLayer, depth);
}

void TileRenderer::submitTile(float x, float y, float width, float height,
                               LPDIRECT3DTEXTURE9 texture, WORD textureID,
                               float u0, float v0, float u1, float v1,
                               WORD shaderID, BYTE blendMode, BYTE layer, WORD depth) {
    if (!m_renderQueue || !texture) return;

    Graphics::RenderCommand cmd = {};
    cmd.x = x;
    cmd.y = y;
    cmd.width = width;
    cmd.height = height;
    cmd.u0 = u0;
    cmd.v0 = v0;
    cmd.u1 = u1;
    cmd.v1 = v1;
    cmd.color = 0xFFFFFFFF;
    cmd.shaderID = shaderID;
    cmd.textureID = textureID;
    cmd.blendMode = blendMode;
    cmd.layer = layer;
    cmd.depth = depth;
    m_renderQueue->Submit(cmd);
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