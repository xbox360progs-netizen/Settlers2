#include "stdafx.h"
#include "Map.h"
#include "../Graphics/Camera.h"
#include "../Logic/CoordinateSystem.h"

namespace World {

Map::Map(int groundWidth, int groundHeight, int otherWidth, int otherHeight)
    : m_width(groundWidth)
    , m_height(groundHeight)
{
    m_layers.resize(static_cast<int>(LayerCount), NULL);

    // Ground layer: 20x20
    m_layers[static_cast<int>(Ground)] = new TileLayer(Ground, groundWidth, groundHeight);

    // Other layers: 40x40
    m_layers[static_cast<int>(Objects)] = new TileLayer(Objects, otherWidth, otherHeight);
    m_layers[static_cast<int>(Overlay)] = new TileLayer(Overlay, otherWidth, otherHeight);
}

Map::~Map()
{
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

    // Objects and Overlay are 40x40
    for (int i = 1; i < static_cast<int>(LayerCount); ++i) {
        if (m_layers[i]) {
            m_layers[i]->Resize(40, 40);
        }
    }
}

void Map::Clear()
{
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
                        tile.type = World::None;
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
    int layerHeight = (layer == Ground) ? m_height : m_height * 2;
    
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

            // Get world position of this tile's center
            float tileCenterX, tileCenterY;
            coords.NodeTileToWorld(checkX, checkY, tileCenterX, tileCenterY);

            // Calculate distance from click point to tile center
            float distX = worldX - tileCenterX;
            float distY = worldY - tileCenterY;
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
        int layerHeight = m_height * 2;
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

} // namespace World
