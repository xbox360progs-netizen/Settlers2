#ifndef SETTLERS2_GRAPHICS_TILE_RENDERER_H
#define SETTLERS2_GRAPHICS_TILE_RENDERER_H

#include "../World/TileType.h"
#include "../World/TileLayer.h"
#include "../World/Map.h"

// Renderer is in global namespace
class Renderer;
class SpriteAtlas;
struct SpriteRegion;

struct IsoTransform {
    static const float TILE_WIDTH;
    static const float TILE_HEIGHT;
    static const float TILE_DEPTH;

    static void worldToScreen(int tileX, int tileY, int tileZ,
                              float& screenX, float& screenY,
                              float offsetX = 0.0f, float offsetY = 0.0f);
    static void screenToWorld(float screenX, float screenY,
                              int& tileX, int& tileY,
                              float offsetX = 0.0f, float offsetY = 0.0f);
    static float getScreenWidth() { return TILE_WIDTH; }
    static float getScreenHeight() { return TILE_HEIGHT; }
};

class TileRenderer {
public:
    TileRenderer(Renderer* renderer, int mapWidth, int mapHeight);
    ~TileRenderer();

    void SetMap(World::Map* map) { m_map = map; }
    void RenderMap(float cameraX = 0.0f, float cameraY = 0.0f, float zoom = 1.0f);
    void RenderTileLayer(World::LayerType layer, int layerOffset = 0);
    void RenderTile(int tileX, int tileY, World::TileType type, int layerOffset = 0);

    void SetProjectionMode(int mode) { m_mode = mode; }
    void WorldToScreen(int wx, int wy, int& sx, int& sy);
    void ScreenToWorld(int sx, int sy, int& wx, int& wy);

    std::pair<int, int> screenToTileCoords(float screenX, float screenY) const;

private:
    void drawTileQuad(float x, float y, float width, float height,
                      LPDIRECT3DTEXTURE9 texture,
                      float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f);

    Renderer* m_renderer;
    World::Map* m_map;
    int m_mapWidth;
    int m_mapHeight;
    int m_mode;
    float m_offsetX, m_offsetY;
    float m_zoom;
};

#endif // SETTLERS2_GRAPHICS_TILE_RENDERER_H
