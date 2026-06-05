#ifndef SETTLERS2_GRAPHICS_TILE_RENDERER_H
#define SETTLERS2_GRAPHICS_TILE_RENDERER_H

#include "../World/TileType.h"
#include "../World/TileLayer.h"
#include "../World/Map.h"
#include "RenderQueue.h"
#include "RenderTypes.h"
#include "RenderLayers.h"
#include "TextureRegistry.h"
#include "../Logic/CoordinateSystem.h"
#include <map>

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
    void RenderMap();
    void RenderTileLayer(World::LayerType layer, int layerOffset = 0);
    void RenderTile(int tileX, int tileY, World::TileType type, int layerOffset = 0);

    void SetProjectionMode(int mode) { m_mode = mode; }
    void SetRenderQueue(Graphics::RenderQueue* rq) { m_renderQueue = rq; }
    void WorldToScreen(int wx, int wy, int& sx, int& sy);
    void ScreenToWorld(int sx, int sy, int& wx, int& wy);

    void SetAtlasSlot(const std::string& atlasName, WORD slot) { m_atlasSlots[atlasName] = slot; }
    bool HasAtlasSlot(const std::string& atlasName) const { return m_atlasSlots.find(atlasName) != m_atlasSlots.end(); }
    void ClearAtlasSlots() { m_atlasSlots.clear(); }

    std::pair<int, int> screenToTileCoords(float screenX, float screenY) const;

private:
    void submitTile(float x, float y, float width, float height,
                    LPDIRECT3DTEXTURE9 texture, WORD textureID,
                    float u0, float v0, float u1, float v1,
                    WORD shaderID, BYTE blendMode, BYTE layer, WORD depth);

    Renderer* m_renderer;
    World::Map* m_map;
    Graphics::RenderQueue* m_renderQueue;
    int m_mapWidth;
    int m_mapHeight;
    int m_mode;
    std::map<std::string, WORD> m_atlasSlots;
};

#endif // SETTLERS2_GRAPHICS_TILE_RENDERER_H