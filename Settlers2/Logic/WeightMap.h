#pragma once

#include "../World/TileLayer.h"
#include <vector>

namespace Logic {

enum TileWeight {
    WEIGHT_IMPASSABLE = 0,
    WEIGHT_DEEP_WATER = 1,
    WEIGHT_SHALLOW_WATER = 2,
    WEIGHT_COAST = 3,
    WEIGHT_LAND = 4,
    WEIGHT_ROAD = 5,
    WEIGHT_DEFAULT = WEIGHT_LAND
};

class WeightMap {
public:
    WeightMap();
    WeightMap(int width, int height);
    ~WeightMap();

    void Resize(int width, int height);

    TileWeight GetWeight(int x, int y) const;
    void SetWeight(int x, int y, TileWeight weight);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    bool IsPassable(int x, int y) const;
    float GetMovementCost(int x, int y) const;

    void Clear();

    static TileWeight GetDefaultWeight(World::TileType type);
    static float GetWeightCost(TileWeight weight);

private:
    int m_width;
    int m_height;
    std::vector<TileWeight> m_weights;
};

} // namespace Logic