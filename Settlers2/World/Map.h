#pragma once

#include "TileLayer.h"
#include <vector>
#include <d3dx9math.h>

class Camera;

namespace World {

class Map
{
public:
    Map(int groundWidth, int groundHeight, int otherWidth, int otherHeight);
    ~Map();

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    TileLayer* GetLayer(LayerType type);
    const TileLayer* GetLayer(LayerType type) const;

    Tile& GetTile(LayerType layer, int x, int y);
    const Tile& GetTile(LayerType layer, int x, int y) const;

    void SetTileType(LayerType layer, int x, int y, TileType type);

    void Resize(int width, int height);
    void Clear();

    // Grid Picking: Get tile under mouse cursor
    // Returns true if a tile was found, false otherwise
    bool GetTileUnderMouse(float screenX, float screenY, Camera* camera, LayerType layer, int& tileX, int& tileY);

    // Get tile at world coordinates (simpler version, no camera needed)
    // Returns true if a tile was found, false otherwise
    bool GetTileAt(float worldX, float worldY, LayerType layer, int& tileX, int& tileY);

    // Get tiles in view for frustum culling
    void GetTilesInView(Camera* camera, LayerType layer, int& minX, int& minY, int& maxX, int& maxY);

private:
    int m_width;
    int m_height;
    std::vector<TileLayer*> m_layers;
};

} // namespace World
