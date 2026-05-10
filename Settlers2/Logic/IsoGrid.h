#pragma once

#include "WeightMap.h"
#include "../World/TileLayer.h"
#include <vector>

namespace Logic {

struct IsoCell {
    int gridX;
    int gridY;
    TileWeight weight;
    bool valid;

    IsoCell() : gridX(0), gridY(0), weight(WEIGHT_LAND), valid(false) {}
};

class IsoGrid {
public:
    IsoGrid();
    ~IsoGrid();

    void BuildFromMap(const World::TileLayer* groundLayer, int terrainTileW, int terrainTileH);

    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

    const IsoCell& GetCell(int x, int y) const;
    bool IsValid(int x, int y) const;
    bool IsPassable(int x, int y) const;
    float GetCost(int x, int y) const;

    void ScreenToIso(float screenX, float screenY, int& isoX, int& isoY) const;
    void IsoToScreen(int isoX, int isoY, float& screenX, float& screenY) const;

    void WorldToIso(float worldX, float worldY, int& isoX, int& isoY) const;
    void IsoToWorld(int isoX, int isoY, float& worldX, float& worldY) const;

    void SetWeightMap(WeightMap* weightMap) { m_weightMap = weightMap; }

private:
    int m_width;
    int m_height;
    std::vector<IsoCell> m_cells;
    WeightMap* m_weightMap;

    int m_terrainTileW;
    int m_terrainTileH;
};

} // namespace Logic