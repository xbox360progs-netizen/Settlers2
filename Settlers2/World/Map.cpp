#include "stdafx.h"
#include "Map.h"
#include "../Graphics/Camera.h"
#include "../Graphics/TileRenderer.h"
#include "../Logic/CoordinateSystem.h"
#include "../Logic/ResourceRegistry.h"
#include "../World/AnimalHabitat.h"
#include "../World/AnimalTypes.h"
#include "TileLayer.h"
#include "../Graphics/Camera.h"
#include "HabitatRegistry.h"

namespace World {

Map::Map(int groundWidth, int groundHeight, int otherWidth, int otherHeight)
    : m_width(groundWidth)
    , m_height(groundHeight)
    , m_resourceRegistry(NULL)
    , m_cargoManager(NULL)
    , m_demandManager(NULL)
{
    InitializeCriticalSection(&m_cs);
    m_layers.resize(static_cast<int>(LayerCount), NULL);

    // Ground layer: 20x20
    m_layers[static_cast<int>(Ground)] = new TileLayer(Ground, groundWidth, groundHeight);

    // Node-based layers: 40x40 (staggered grid)
    m_layers[static_cast<int>(Roads)] = new TileLayer(Roads, otherWidth, otherHeight);
    m_layers[static_cast<int>(Nodes)] = new TileLayer(Nodes, otherWidth, otherHeight);
    m_layers[static_cast<int>(Placement)] = new TileLayer(Placement, otherWidth, otherHeight);
    m_layers[static_cast<int>(Resources)] = new TileLayer(Resources, otherWidth, otherHeight);
    m_layers[static_cast<int>(Objects)] = new TileLayer(Objects, otherWidth, otherHeight);
    m_layers[static_cast<int>(Overlay)] = new TileLayer(Overlay, otherWidth, otherHeight);
    m_layers[static_cast<int>(Buildings)] = new TileLayer(Buildings, otherWidth, otherHeight);

    // Initialize resource map (same size as Objects layer: 40x40)
    int resourceMapSize = otherWidth * otherHeight;
    m_resourceMap.resize(resourceMapSize, World::ResourceNode());
    m_nodes.resize(resourceMapSize, World::MapNode());

    // Initialize weights to default (Land = 2)
    InitializeWeights(Weight_Deep);
}

Map::~Map()
{
    DeleteCriticalSection(&m_cs);
    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        delete m_layers[i];
        m_layers[i] = NULL;
    }
}

TileLayer* Map::GetLayer(LayerType type)
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(m_layers.size()))
    {
        return m_layers[index];
    }
    return NULL;
}

const TileLayer* Map::GetLayer(LayerType type) const
{
    int index = static_cast<int>(type);
    if (index >= 0 && index < static_cast<int>(m_layers.size()))
    {
        return m_layers[index];
    }
    return NULL;
}

Tile& Map::GetTile(LayerType layer, int x, int y)
{
    TileLayer* tileLayer = GetLayer(layer);
    if (tileLayer)
    {
        return tileLayer->GetTile(x, y);
    }
    static Tile invalidTile;
    return invalidTile;
}

const Tile& Map::GetTile(LayerType layer, int x, int y) const
{
    const TileLayer* tileLayer = GetLayer(layer);
    if (tileLayer)
    {
        return tileLayer->GetTile(x, y);
    }
    static Tile invalidTile;
    return invalidTile;
}

void Map::SetTileType(LayerType layer, int x, int y, TileType type)
{
    TileLayer* tileLayer = GetLayer(layer);
    if (tileLayer)
    {
        tileLayer->SetTileType(x, y, type);
    }
}

void Map::Resize(int width, int height)
{
    // Ground remains 20x20
    if (m_layers[static_cast<int>(Ground)]) {
        m_layers[static_cast<int>(Ground)]->Resize(20, 20);
    }

    // Objects and Overlay are 40x80 (denser grid)
    for (int i = 1; i < static_cast<int>(LayerCount); ++i) {
        if (m_layers[i]) {
            m_layers[i]->Resize(m_width * 2, m_height * 4);
        }
    }
}

void Map::Clear()
{
	if (m_resourceRegistry)
		m_resourceRegistry->ClearWorldResources();
    m_habitatRegistry.Clear();

    for (size_t i = 0; i < m_layers.size(); ++i)
    {
        if (m_layers[i])
        {
            for (int y = 0; y < m_height; ++y)
            {
                for (int x = 0; x < m_width; ++x)
                {
                    World::Tile& tile = m_layers[i]->GetTile(x, y);
                    tile.regionIndex = 0;
                    // Don't set type for Ground!
                    if (m_layers[i]->GetType() != World::Ground) {
                        tile.type = World::Tile_None;
                    }
                }
            }
        }
    }
}

// Grid Picking: Get tile under mouse cursor using nearest center algorithm
bool Map::GetTileUnderMouse(float screenX, float screenY, Camera* camera, LayerType layer, int& tileX, int& tileY)
{
    if (!camera) return false;

    // Convert screen coordinates to world coordinates
    float worldX, worldY;
    camera->ScreenToWorld(screenX, screenY, worldX, worldY);

    // Get coordinate system instance
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    // For Ground layer (Layer 0): use simple grid
    if (layer == Ground) {
        coords.WorldToGroundTile(worldX, worldY, tileX, tileY);
        
        // Check bounds
        if (tileX < 0 || tileX >= m_width || tileY < 0 || tileY >= m_height) {
            return false;
        }
        return true;
    }

    // For Objects/Overlay layers (Layer 1): use staggered grid with nearest center
    // Get initial estimate
    int initialX, initialY;
    coords.WorldToNodeTile(worldX, worldY, initialX, initialY);

    // Check bounds
    int layerWidth = (layer == Ground) ? m_width : m_width * 2;
    int layerHeight = (layer == Ground) ? m_height : m_height * 4;
    
    if (initialX < 0 || initialX >= layerWidth || initialY < 0 || initialY >= layerHeight) {
        return false;
    }

    // Nearest center algorithm: check the initial tile and its neighbors
    float minDist = FLT_MAX;
    int bestX = initialX;
    int bestY = initialY;

    // Check 3x3 neighborhood around initial estimate
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int checkX = initialX + dx;
            int checkY = initialY + dy;

            // Skip out of bounds
            if (checkX < 0 || checkX >= layerWidth || checkY < 0 || checkY >= layerHeight) {
                continue;
            }

            // Get world position of this tile's anchor (matches cursor and object rendering)
            float tileAnchorX, tileAnchorY;
            coords.NodeTileToWorld(checkX, checkY, tileAnchorX, tileAnchorY);

            // Use cell center for distance comparison (staggered grid cells overlap at corners)
            float cellCenterX = tileAnchorX + coords.GetNodeWidth() * 0.5f;
            float cellCenterY = tileAnchorY + coords.GetNodeHeight() * 0.5f;
            float distX = worldX - cellCenterX;
            float distY = worldY - cellCenterY;
            float dist = sqrtf(distX * distX + distY * distY);

            // Track closest tile
            if (dist < minDist) {
                minDist = dist;
                bestX = checkX;
                bestY = checkY;
            }
        }
    }

    tileX = bestX;
    tileY = bestY;
    return true;
}

// Get tile at world coordinates (simpler version, no camera needed)
bool Map::GetTileAt(float worldX, float worldY, LayerType layer, int& tileX, int& tileY)
{
    // Get coordinate system instance
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    // For Ground layer (Layer 0): use simple grid
    if (layer == Ground) {
        coords.WorldToGroundTile(worldX, worldY, tileX, tileY);
        
        // Check bounds
        if (tileX < 0 || tileX >= m_width || tileY < 0 || tileY >= m_height) {
            return false;
        }
        return true;
    }

    // For Objects/Overlay layers (Layer 1): use staggered grid with nearest center
    // Get initial estimate
    int initialX, initialY;
    coords.WorldToNodeTile(worldX, worldY, initialX, initialY);

    // Check bounds
    int layerWidth = (layer == Ground) ? m_width : m_width * 2;
    int layerHeight = (layer == Ground) ? m_height : m_height * 4;
    
    if (initialX < 0 || initialX >= layerWidth || initialY < 0 || initialY >= layerHeight) {
        return false;
    }

    // Nearest center algorithm: check the initial tile and its neighbors
    float minDist = FLT_MAX;
    int bestX = initialX;
    int bestY = initialY;

    // Check 3x3 neighborhood around initial estimate
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int checkX = initialX + dx;
            int checkY = initialY + dy;

            // Skip out of bounds
            if (checkX < 0 || checkX >= layerWidth || checkY < 0 || checkY >= layerHeight) {
                continue;
            }

            // Get world position of this tile's anchor (matches cursor and object rendering)
            float tileAnchorX, tileAnchorY;
            coords.NodeTileToWorld(checkX, checkY, tileAnchorX, tileAnchorY);

            // Use cell center for distance comparison (staggered grid cells overlap at corners)
            float cellCenterX = tileAnchorX + coords.GetNodeWidth() * 0.5f;
            float cellCenterY = tileAnchorY + coords.GetNodeHeight() * 0.5f;
            float distX = worldX - cellCenterX;
            float distY = worldY - cellCenterY;
            float dist = sqrtf(distX * distX + distY * distY);

            // Track closest tile
            if (dist < minDist) {
                minDist = dist;
                bestX = checkX;
                bestY = checkY;
            }
        }
    }

    tileX = bestX;
    tileY = bestY;
    return true;
}

// Get tiles in view for frustum culling
void Map::GetTilesInView(Camera* camera, LayerType layer, int& minX, int& minY, int& maxX, int& maxY)
{
    if (!camera) {
        minX = 0;
        minY = 0;
        maxX = m_width;
        maxY = m_height;
        return;
    }

    // Get screen corners in world space
    float worldTL_X, worldTL_Y; // Top-left
    float worldTR_X, worldTR_Y; // Top-right
    float worldBL_X, worldBL_Y; // Bottom-left
    float worldBR_X, worldBR_Y; // Bottom-right

    camera->ScreenToWorld(0.0f, 0.0f, worldTL_X, worldTL_Y);
    camera->ScreenToWorld(camera->GetScreenWidth(), 0.0f, worldTR_X, worldTR_Y);
    camera->ScreenToWorld(0.0f, camera->GetScreenHeight(), worldBL_X, worldBL_Y);
    camera->ScreenToWorld(camera->GetScreenWidth(), camera->GetScreenHeight(), worldBR_X, worldBR_Y);

    // Find bounding box in world coordinates
    float worldMinX = min(min(worldTL_X, worldTR_X), min(worldBL_X, worldBR_X));
    float worldMaxX = max(max(worldTL_X, worldTR_X), max(worldBL_X, worldBR_X));
    float worldMinY = min(min(worldTL_Y, worldTR_Y), min(worldBL_Y, worldBR_Y));
    float worldMaxY = max(max(worldTL_Y, worldTR_Y), max(worldBL_Y, worldBR_Y));

    // Convert to tile coordinates
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    if (layer == Ground) {
        coords.WorldToGroundTile(worldMinX, worldMinY, minX, minY);
        coords.WorldToGroundTile(worldMaxX, worldMaxY, maxX, maxY);
        
        // Clamp to map bounds
        minX = max(0, minX);
        minY = max(0, minY);
        maxX = min(m_width - 1, maxX);
        maxY = min(m_height - 1, maxY);
    } else {
        coords.WorldToNodeTile(worldMinX, worldMinY, minX, minY);
        coords.WorldToNodeTile(worldMaxX, worldMaxY, maxX, maxY);
        
        // Clamp to layer bounds (40x40 for Objects/Overlay)
        int layerWidth = m_width * 2;
        int layerHeight = m_height * 4;
        minX = max(0, minX);
        minY = max(0, minY);
        maxX = min(layerWidth - 1, maxX);
        maxY = min(layerHeight - 1, maxY);
    }

    // Add padding to avoid edge popping
    minX = max(0, minX - 1);
    minY = max(0, minY - 1);
    maxX = maxX + 1;
    maxY = maxY + 1;
}

// Resource management
ResourceNode& Map::GetResourceNode(int x, int y)
{
    int layerWidth = m_width * 2;  // Objects layer is 40x40
    int layerHeight = m_height * 4;
    
    if (x < 0 || x >= layerWidth || y < 0 || y >= layerHeight) {
        static ResourceNode invalidNode;
        return invalidNode;
    }
    
    int index = y * layerWidth + x;
    if (index >= 0 && index < static_cast<int>(m_resourceMap.size())) {
        return m_resourceMap[index];
    }
    
    static ResourceNode invalidNode;
    return invalidNode;
}

const ResourceNode& Map::GetResourceNode(int x, int y) const
{
    int layerWidth = m_width * 2;  // Objects layer is 40x40
    int layerHeight = m_height * 4;
    
    if (x < 0 || x >= layerWidth || y < 0 || y >= layerHeight) {
        static ResourceNode invalidNode;
        return invalidNode;
    }
    
    int index = y * layerWidth + x;
    if (index >= 0 && index < static_cast<int>(m_resourceMap.size())) {
        return m_resourceMap[index];
    }
    
    static ResourceNode invalidNode;
    return invalidNode;
}

void Map::SetResourceNode(int x, int y, ResourceType type, int amount, bool isVisible)
{
    int layerWidth = m_width * 2;  // Objects layer is 40x40
    int layerHeight = m_height * 4;
    
    if (x < 0 || x >= layerWidth || y < 0 || y >= layerHeight) {
        return;
    }
    
    int index = y * layerWidth + x;
    if (index >= 0 && index < static_cast<int>(m_resourceMap.size())) {
        ResourceType oldType = m_resourceMap[index].type;
        m_resourceMap[index].type = type;
        m_resourceMap[index].amount = amount;
        m_resourceMap[index].isVisible = isVisible;

        // Keep ResourceRegistry in sync at runtime
        if (m_resourceRegistry) {
            if (oldType != ResourceType_None && oldType != type) {
                m_resourceRegistry->UnregisterWorldResource(oldType, x, y);
                // Also unregister spawner counterpart if applicable
                if (oldType == ResourceType_Meat)
                    m_resourceRegistry->UnregisterWorldResource(ResourceType_WildlifeSpawner_Deer, x, y);
            }
            m_resourceRegistry->RegisterWorldResource(type, x, y);
            // Register WildlifeSpawner counterpart so hunters/buildings can find it
            if (type == ResourceType_Meat)
                m_resourceRegistry->RegisterWorldResource(ResourceType_WildlifeSpawner_Deer, x, y);
        }
        
        // Register habitat if type is a wildlife spawner
        if (type != oldType) {
            World::AnimalType animalType;
            bool isSpawner = true;
            switch (type) {
                case ResourceType_WildlifeSpawner_Deer:      animalType = AnimalType_Deer; break;
                case ResourceType_WildlifeSpawner_Rabbit:    animalType = AnimalType_Rabbit; break;
                case ResourceType_WildlifeSpawner_Crocodile: animalType = AnimalType_Crocodile; break;
                case ResourceType_WildlifeSpawner_Snake:     animalType = AnimalType_Snake; break;
                case ResourceType_Meat:                      animalType = AnimalType_Deer; break;
                default: isSpawner = false; break;
            }
            if (isSpawner) {
                World::AnimalHabitat hab;
                hab.center.x = x;
                hab.center.y = y;
                hab.type = animalType;
                hab.maxAnimals = (amount > 0) ? amount : 5;
                hab.radius = 10;
                m_habitatRegistry.Register(hab);
            }
        }
    }
}

void Map::ClearResources()
{
    if (m_resourceRegistry) {
        int w = m_width * 2;
        int h = m_height * 4;
        for (int y = 0; y < h; ++y)
            for (int x = 0; x < w; ++x) {
                ResourceType rt = m_resourceMap[y * w + x].type;
                if (rt != ResourceType_None)
                    m_resourceRegistry->UnregisterWorldResource(rt, x, y);
                if (rt == ResourceType_Meat)
                    m_resourceRegistry->UnregisterWorldResource(ResourceType_WildlifeSpawner_Deer, x, y);
            }
    }
    for (size_t i = 0; i < m_resourceMap.size(); ++i) {
        m_resourceMap[i] = World::ResourceNode();
    }
}

void Map::SetResourceRegistry(Logic::ResourceRegistry* rr) {
    m_resourceRegistry = rr;
    if (m_resourceRegistry) {
        // Register all existing resource nodes so the registry is in sync
        int w = m_width * 2;
        int h = m_height * 4;
        for (int y = 0; y < h; ++y) {
            for (int x = 0; x < w; ++x) {
                const ResourceNode& node = m_resourceMap[y * w + x];
                if (node.type != ResourceType_None) {
                    m_resourceRegistry->RegisterWorldResource(node.type, x, y);
                    if (node.type == ResourceType_Meat)
                        m_resourceRegistry->RegisterWorldResource(ResourceType_WildlifeSpawner_Deer, x, y);
                }
            }
        }
    }
}

bool Map::FindResourceInRadius(int centerX, int centerY, int radius, ResourceType type, int& foundX, int& foundY) const
{
    EnterCriticalSection(&const_cast<Map*>(this)->m_cs);
    int layerWidth = m_width * 2;
    int layerHeight = m_height * 4;

    bool found = false;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int checkX = centerX + dx;
            int checkY = centerY + dy;

            if (checkX >= 0 && checkX < layerWidth && checkY >= 0 && checkY < layerHeight) {
                const ResourceNode& node = m_resourceMap[checkY * layerWidth + checkX];
                if (node.type == type) {
                    foundX = checkX;
                    foundY = checkY;
                    found = true;
                    goto end;
                }
            }
        }
    }
end:
    LeaveCriticalSection(&const_cast<Map*>(this)->m_cs);
    return found;
}

bool Map::FindTileTypeInRadius(int centerX, int centerY, int radius, LayerType layer, TileType type, int& foundX, int& foundY) const
{
    EnterCriticalSection(&const_cast<Map*>(this)->m_cs);
    TileLayer* tileLayer = const_cast<Map*>(this)->GetLayer(layer);
    if (!tileLayer) {
        LeaveCriticalSection(&const_cast<Map*>(this)->m_cs);
        return false;
    }
    
    int layerWidth = tileLayer->GetWidth();
    int layerHeight = tileLayer->GetHeight();

    bool found = false;
    for (int dy = -radius; dy <= radius; ++dy) {
        for (int dx = -radius; dx <= radius; ++dx) {
            int checkX = centerX + dx;
            int checkY = centerY + dy;

            if (checkX >= 0 && checkX < layerWidth && checkY >= 0 && checkY < layerHeight) {
                if (tileLayer->GetTile(checkX, checkY).type == type) {
                    foundX = checkX;
                    foundY = checkY;
                    found = true;
                    goto end_tile;
                }
            }
        }
    }
end_tile:
    LeaveCriticalSection(&const_cast<Map*>(this)->m_cs);
    return found;
}

// Weight management
BYTE Map::GetNodeWeight(int x, int y) const
{
    int layerWidth = m_width * 2;   // 40
    int layerHeight = m_height * 4; // 80 (double rows at half spacing)
    
    if (x < 0 || x >= layerWidth || y < 0 || y >= layerHeight) {
        return Weight_Land;  // Default to land if out of bounds
    }
    
    int index = y * layerWidth + x;
    if (index >= 0 && index < static_cast<int>(m_resourceMap.size())) {
        return m_resourceMap[index].weight;
    }
    
    return Weight_Land;
}

void Map::SetNodeWeight(int x, int y, BYTE weight)
{
    int layerWidth = m_width * 2;   // 40
    int layerHeight = m_height * 4; // 80
    
    if (x < 0 || x >= layerWidth || y < 0 || y >= layerHeight) {
        return;
    }
    
    int index = y * layerWidth + x;
    if (index >= 0 && index < static_cast<int>(m_resourceMap.size())) {
        m_resourceMap[index].weight = weight;
    }
}

void Map::InitializeWeights(BYTE defaultWeight)
{
    for (size_t i = 0; i < m_resourceMap.size(); ++i) {
        m_resourceMap[i].weight = defaultWeight;
    }
}

void Map::RecalculateTerritory()
{
    // TODO: Implement territory expansion algorithm (e.g., flood fill from buildings)
}

void Map::SetTileOwner(int x, int y, uint8_t owner)
{
    if (x >= 0 && x < m_width * 2 && y >= 0 && y < m_height * 4) {
        m_nodes[y * (m_width * 2) + x].owner = owner;
    }
}

void Map::GenerateWildlife() {
    World::AnimalHabitat hab;
    hab.center.x = 10;
	hab.center.y = 10;
    hab.type = World::AnimalType_Deer;
    m_habitatRegistry.Register(hab);

    // Generate fish resources on water tiles
    int w = m_width * 2;
    int h = m_height * 4;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int idx = y * w + x;
            BYTE weight = m_resourceMap[idx].weight;
            // Water tiles have Weight_Deep or Weight_Shallow
            if (weight <= Weight_Shallow && m_resourceMap[idx].type == ResourceType_None) {
                SetResourceNode(x, y, ResourceType_Fish, 5, false);
            }
        }
    }
}

void Map::RegenerateWildlifeResources() {
    int w = m_width * 2;
    int h = m_height * 4;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            ResourceNode& node = m_resourceMap[y * w + x];
            if (node.type == ResourceType_WildlifeSpawner_Deer ||
                node.type == ResourceType_WildlifeSpawner_Rabbit ||
                node.type == ResourceType_WildlifeSpawner_Crocodile ||
                node.type == ResourceType_WildlifeSpawner_Snake ||
                node.type == ResourceType_Fish) {
                if (node.amount < 5) {
                    node.amount++;
                }
            }
        }
    }
}

static unsigned int g_groundSpawnFrame = 0;
unsigned int GetNextSpawnFrame() { return ++g_groundSpawnFrame; }

void Map::SpawnGroundResource(ResourceType type, int amount, int x, int y) {
    GroundResource gr;
    gr.type = type;
    gr.amount = amount;
    gr.pos.x = x;
    gr.pos.y = y;
    gr.spawnFrame = GetNextSpawnFrame();
    gr.visualOnly = (type == ResourceType_Wood);
    m_groundResources.push_back(gr);
    char dbg[128];
    _snprintf(dbg, sizeof(dbg), "[Ground] %s spawned (%d,%d) amount=%d frame=%u\n",
              (type == ResourceType_Wood) ? "Wood" : "Resource", x, y, amount, gr.spawnFrame);
    OutputDebugStringA(dbg);
}

GroundResource* Map::GetGroundResource(int index) {
    if (index < 0 || index >= (int)m_groundResources.size()) return NULL;
    return &m_groundResources[index];
}

void Map::RemoveGroundResource(int index) {
    if (index >= 0 && index < (int)m_groundResources.size())
        m_groundResources.erase(m_groundResources.begin() + index);
}

void Map::ClearGroundResources() {
    m_groundResources.clear();
}

bool Map::RemoveGroundResourceAt(int x, int y) {
    for (size_t i = 0; i < m_groundResources.size(); ++i) {
        if (m_groundResources[i].pos.x == x && m_groundResources[i].pos.y == y) {
            m_groundResources.erase(m_groundResources.begin() + i);
            return true;
        }
    }
    return false;
}

GroundResource* Map::FindGroundResourceAt(int x, int y) {
    for (size_t i = 0; i < m_groundResources.size(); ++i) {
        if (m_groundResources[i].pos.x == x && m_groundResources[i].pos.y == y)
            return &m_groundResources[i];
    }
    return NULL;
}

void Map::SetStumpSpriteIndices(int idx1, int idx2, int idx3) {
    m_stumpIndices[0] = idx1;
    m_stumpIndices[1] = idx2;
    m_stumpIndices[2] = idx3;
}

void Map::SetTileAsStump(int x, int y) {
    TileLayer* objectsLayer = GetLayer(Objects);
    if (!objectsLayer) return;
    Tile& tile = objectsLayer->GetTile(x, y);
    int si = rand() % 3;
    tile.regionIndex = m_stumpIndices[si];
    tile.atlasName = "maptiles";
    tile.type = Decoration;
    tile.UpdateProperties();
}

void Map::GrowTrees() {
    int w = m_width * 2;
    int h = m_height * 4;
    int grown = 0, decayed = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            ResourceNode& node = m_resourceMap[y * w + x];
            if (!IsTree(node.type)) continue;
            if (IsTreeAlive(node.amount) && node.amount < TreeState_Mature) {
                node.amount++;
                ++grown;
            } else if (IsTreeStump(node.amount)) {
                node.amount = TreeState_Empty;
                ++decayed;
            }
        }
    }
    if (grown > 0 || decayed > 0) {
        char dbg[128];
        _snprintf(dbg, sizeof(dbg), "[GrowTrees] advanced=%d stumpDecayed=%d\n", grown, decayed);
        OutputDebugStringA(dbg);
    }
}

} // namespace World
