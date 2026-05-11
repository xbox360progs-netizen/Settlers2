#pragma once

#include <vector>
#include "TileLayer.h"
#include "TileType.h"
#include "ResourceNode.h"
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

    // Resource management
    ResourceNode& GetResourceNode(int x, int y);
    const ResourceNode& GetResourceNode(int x, int y) const;
    void SetResourceNode(int x, int y, ResourceType type, int amount, bool isVisible = true);
    void ClearResources();

    // Weight management
    BYTE GetNodeWeight(int x, int y) const;
    void SetNodeWeight(int x, int y, BYTE weight);
    void InitializeWeights(BYTE defaultWeight = Weight_Land);

private:
    int m_width;
    int m_height;
    std::vector<TileLayer*> m_layers;

    // Resource map for the logical grid (staggered, same size as Objects layer)
    std::vector<ResourceNode> m_resourceMap;
};

} // namespace World
