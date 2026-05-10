#include "stdafx.h"
#include "TileLayer.h"

namespace World {

TileLayer::TileLayer(LayerType type, int width, int height)
    : m_type(type)
    , m_width(width)
    , m_height(height)
{
    m_tiles.resize(width * height);
}

TileLayer::~TileLayer()
{
}

Tile& TileLayer::GetTile(int x, int y)
{
    int index = GetIndex(x, y);
    return m_tiles[index];
}

const Tile& TileLayer::GetTile(int x, int y) const
{
    int index = GetIndex(x, y);
    return m_tiles[index];
}

void TileLayer::SetTile(int x, int y, const Tile& tile)
{
    int index = GetIndex(x, y);
    m_tiles[index] = tile;
}

void TileLayer::SetTileType(int x, int y, TileType type)
{
    int index = GetIndex(x, y);
    m_tiles[index].type = type;
    m_tiles[index].x = x;
    m_tiles[index].y = y;
    m_tiles[index].UpdateProperties();
}

int TileLayer::GetIndex(int x, int y) const
{
    return y * m_width + x;
}

void TileLayer::Resize(int width, int height)
{
    std::vector<Tile> newTiles(width * height);

    // Copy existing tiles
    for (int y = 0; y < min(m_height, height); ++y)
    {
        for (int x = 0; x < min(m_width, width); ++x)
        {
            newTiles[y * width + x] = m_tiles[y * m_width + x];
        }
    }

    m_tiles = newTiles;
    m_width = width;
    m_height = height;
}

} // namespace World
