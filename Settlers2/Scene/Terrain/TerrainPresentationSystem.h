#pragma once
#include <vector>

namespace World {
    class Map;
}

namespace Scene {

struct RenderFrame;
struct RenderTerrainTile;

// Reads World::Map tile layers (Ground, Roads, Objects, Buildings) and
// produces per-tile RenderTerrainTile DTOs with pre-computed world coords,
// sprite dimensions, UVs, and depth. Runs during GameScene::Update().
class TerrainPresentationSystem {
public:
    TerrainPresentationSystem();
    ~TerrainPresentationSystem();

    void SetMap(World::Map* map, int mapWidth, int mapHeight);

    // Read map tiles → append RenderTerrainTile DTOs to frame.terrain.
    void BuildRenderFrame(RenderFrame& frame);

private:
    // Resolve a single layer into DTOs. layerType cast to World::LayerType.
    void ResolveLayer(int layerType, std::vector<RenderTerrainTile>& outTiles);

    World::Map* m_map;
    int m_mapWidth;
    int m_mapHeight;
};

}
