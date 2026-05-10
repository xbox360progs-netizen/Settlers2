#include "stdafx.h"
#include "Map.h"

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
    // Ground остаётся 20x20
    if (m_layers[static_cast<int>(Ground)]) {
        m_layers[static_cast<int>(Ground)]->Resize(20, 20);
    }

    // Objects и Overlay — 40x40
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
					// Не устанавливаем type для Ground!
					if (m_layers[i]->GetType() != World::Ground) {
						tile.type = World::None; 
}
                }
            }
        }
    }
}

} // namespace World
