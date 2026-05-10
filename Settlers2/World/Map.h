#pragma once

#include "TileLayer.h"
#include <vector>

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

private:
    int m_width;
    int m_height;
    std::vector<TileLayer*> m_layers;
};

} // namespace World
