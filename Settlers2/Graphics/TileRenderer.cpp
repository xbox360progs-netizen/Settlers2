#include "stdafx.h"
#include "TileRenderer.h"
#include "Renderer.h"
#include "TextureRegistry.h"
#include "Texture.h"
#include "SpriteAtlas.h"
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
  : m_renderer(renderer), m_map(nullptr), m_mapWidth(mapWidth), m_mapHeight(mapHeight),
    m_mode(0), m_offsetX(0), m_offsetY(0), m_zoom(1.0f) {}

TileRenderer::~TileRenderer() {}

void TileRenderer::RenderMap(float cameraX, float cameraY, float zoom) {
    if (!m_map) return;

    m_offsetX = cameraX;
    m_offsetY = cameraY;
    m_zoom = zoom;

    // Render each layer in correct order
    for (int layerType = 0; layerType < static_cast<int>(World::LayerCount); ++layerType) {
        World::TileLayer* layer = m_map->GetLayer(static_cast<World::LayerType>(layerType));
        if (layer) {
            RenderTileLayer(static_cast<World::LayerType>(layerType), layerType);
        }
    }
}

void TileRenderer::RenderTileLayer(World::LayerType layer, int layerOffset) {
    if (!m_map) return;

    World::TileLayer* tileLayer = m_map->GetLayer(layer);
    if (!tileLayer) return;

    int width = tileLayer->GetWidth();
    int height = tileLayer->GetHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const World::Tile& tile = tileLayer->GetTile(x, y);
            if (tile.type != World::None) {
                RenderTile(x, y, tile.type, layerOffset);
            }
        }
    }
}

void TileRenderer::RenderTile(int tileX, int tileY, World::TileType type, int layerOffset) {
    (void)tileX;
    (void)tileY;
    (void)type;
    (void)layerOffset;
}

void TileRenderer::drawTileQuad(float x, float y, float width, float height,
                                 LPDIRECT3DTEXTURE9 texture,
                                 float u0, float v0, float u1, float v1) {
    if (!m_renderer || !texture) return;

    ::Renderer* renderer = m_renderer;
    if (!renderer) return;

    Texture tempTex;
    tempTex.SetTexture(texture);
    renderer->DrawSingleSprite(&tempTex, x, y, width, height);
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
    IsoTransform::screenToWorld(screenX, screenY, tileX, tileY, m_offsetX, m_offsetY);
    return std::make_pair(tileX, tileY);
}
