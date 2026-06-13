#pragma once

#include <vector>
#include <xtl.h> // For CRITICAL_SECTION
#include "TileLayer.h"
#include "TileType.h"
#include "ResourceNode.h"
#include "MapNode.h"
#include <d3dx9math.h>
#include "HabitatRegistry.h"
class Camera;

namespace Logic {
    class ResourceRegistry;
}

namespace World {

class WildlifeSystem;

using ::Camera;


class Map
{
public:
    Map(int groundWidth, int groundHeight, int otherWidth, int otherHeight);
    ~Map();

    void Lock() { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

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
    void GenerateWildlife();

    // Find resource in radius
    bool FindResourceInRadius(int centerX, int centerY, int radius, ResourceType type, int& foundX, int& foundY) const;
    
    // Find tile type in radius
    bool FindTileTypeInRadius(int centerX, int centerY, int radius, LayerType layer, TileType type, int& foundX, int& foundY) const;

    // Weight management
    BYTE GetNodeWeight(int x, int y) const;
    void SetNodeWeight(int x, int y, BYTE weight);
    void InitializeWeights(BYTE defaultWeight = Weight_Land);

    // Territory management
    void RecalculateTerritory();
    void SetTileOwner(int x, int y, uint8_t owner);

    // Wildlife system
    void SetWildlifeSystem(WildlifeSystem* ws) { m_wildlifeSystem = ws; }
    WildlifeSystem* GetWildlifeSystem() const { return m_wildlifeSystem; }

    // Regenerate wildlife resource node amounts
    void RegenerateWildlifeResources();

    // Resource registry
    void SetResourceRegistry(Logic::ResourceRegistry* rr);
    Logic::ResourceRegistry* GetResourceRegistry() const { return m_resourceRegistry; }

    // Habitat registry access (read-only)
    const HabitatRegistry& GetHabitatRegistry() const { return m_habitatRegistry; }
    HabitatRegistry& GetHabitatRegistry() { return m_habitatRegistry; }

private:
    int m_width;
    int m_height;
    std::vector<TileLayer*> m_layers;

	World::HabitatRegistry m_habitatRegistry;

    // Resource map for the logical grid (staggered, same size as Objects layer)
    std::vector<ResourceNode> m_resourceMap;
    std::vector<MapNode> m_nodes;
    
    WildlifeSystem* m_wildlifeSystem;
    Logic::ResourceRegistry* m_resourceRegistry;

    CRITICAL_SECTION m_cs;
};

} // namespace World
