#pragma once

#include "Tile.h"
#include <vector>

namespace World {

enum LayerType
{
    Roads,
    Nodes,
    Placement,
    Resources,
    Ground,
    Objects,
    Overlay,
    LayerCount
};

class TileLayer
{
public:
    TileLayer(LayerType type, int width, int height);
    ~TileLayer();

    Tile& GetTile(int x, int y);
    const Tile& GetTile(int x, int y) const;

    void SetTile(int x, int y, const Tile& tile);
    void SetTileType(int x, int y, TileType type);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    LayerType GetType() const { return m_type; }

    void Resize(int width, int height);

private:
    int GetIndex(int x, int y) const;

    LayerType m_type;
    int m_width;
    int m_height;
    std::vector<Tile> m_tiles;
};

} // namespace World
