#ifndef SETTLERS2_GRAPHICS_TILE_RENDERER_H
#define SETTLERS2_GRAPHICS_TILE_RENDERER_H

#include "RenderTypes.h"
#include "RenderLayers.h"
#include "TextureRegistry.h"
#include "../Logic/CoordinateSystem.h"
#include <map>
#include <vector>
#include <string>

class Renderer;

namespace Scene {
    struct RenderTerrainTile;
    class RenderCommandBuffer;
}

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

    // DTO-based terrain render (Stage 6B — pushes to CommandBuffer).
    // Reads pre-built DTOs from TerrainPresentationSystem; resolves atlas
    // slots from m_atlasSlots (set by atlas binding loop in GameRenderer).
    void RenderTerrainTiles(const std::vector<Scene::RenderTerrainTile>& tiles,
                            Scene::RenderCommandBuffer& buffer);

    void SetProjectionMode(int mode) { m_mode = mode; }
    void WorldToScreen(int wx, int wy, int& sx, int& sy);
    void ScreenToWorld(int sx, int sy, int& wx, int& wy);

    void SetAtlasSlot(const std::string& atlasName, WORD slot) { m_atlasSlots[atlasName] = slot; }
    bool HasAtlasSlot(const std::string& atlasName) const { return m_atlasSlots.find(atlasName) != m_atlasSlots.end(); }
    void ClearAtlasSlots() { m_atlasSlots.clear(); }

    std::pair<int, int> screenToTileCoords(float screenX, float screenY) const;

private:
    Renderer* m_renderer;
    int m_mapWidth;
    int m_mapHeight;
    int m_mode;
    std::map<std::string, WORD> m_atlasSlots;
};

#endif // SETTLERS2_GRAPHICS_TILE_RENDERER_H
