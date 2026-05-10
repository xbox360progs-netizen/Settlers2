#include "stdafx.h"
#include "IsoGrid.h"
#include "MapConstants.h"
#include <cmath>
#include <cstdio>

namespace Logic {

IsoGrid::IsoGrid()
    : m_width(0), m_height(0), m_weightMap(nullptr), m_terrainTileW(238), m_terrainTileH(148) {
}

IsoGrid::~IsoGrid() {
}

void IsoGrid::BuildFromMap(const World::TileLayer* groundLayer, int terrainTileW, int terrainTileH) {
    if (!groundLayer) return;

    m_terrainTileW = terrainTileW;
    m_terrainTileH = terrainTileH;

    int mapW = groundLayer->GetWidth();
    int mapH = groundLayer->GetHeight();

    float worldW = mapW * terrainTileW;
    float worldH = mapH * terrainTileH;

    float minIsoX = -worldH / ISO_HALF_W;
    float maxIsoX = worldW / ISO_HALF_W;
    float minIsoY = 0.0f;
    float maxIsoY = (worldW + worldH) / ISO_HALF_H;

    m_width = static_cast<int>(ceilf(maxIsoX - minIsoX));
    m_height = static_cast<int>(ceilf(maxIsoY - minIsoY));

    m_cells.resize(m_width * m_height);

    // Rectangle mask bounds (centered on map)
    float rectLeft = -worldW * 0.5f;
    float rectTop = -worldH * 0.5f;
    float rectRight = worldW * 0.5f;
    float rectBottom = worldH * 0.5f;

    for (int iy = 0; iy < m_height; ++iy) {
        for (int ix = 0; ix < m_width; ++ix) {
            float isoX = minIsoX + ix;
            float isoY = minIsoY + iy;

            float worldX = (isoX + isoY) * ISO_HALF_W;
            float worldY = (isoY - isoX) * ISO_HALF_H;

            int tileX = static_cast<int>(worldX / terrainTileW);
            int tileY = static_cast<int>(worldY / terrainTileH);

            IsoCell& cell = m_cells[iy * m_width + ix];
            cell.gridX = tileX;
            cell.gridY = tileY;

            // Check if within map bounds AND rectangle mask
            cell.valid = (tileX >= 0 && tileX < mapW && tileY >= 0 && tileY < mapH);
            bool insideRect = (worldX >= rectLeft && worldX <= rectRight &&
                              worldY >= rectTop && worldY <= rectBottom);

            if (cell.valid && insideRect && groundLayer) {
                World::Tile tile = groundLayer->GetTile(tileX, tileY);
                cell.weight = WeightMap::GetDefaultWeight(tile.type);
            } else {
                cell.weight = WEIGHT_IMPASSABLE;
                cell.valid = false;
            }
        }
    }

    char buf[128];
    sprintf_s(buf, "[IsoGrid] Built grid %dx%d from map %dx%d\n", m_width, m_height, mapW, mapH);
    OutputDebugStringA(buf);
}

const IsoCell& IsoGrid::GetCell(int x, int y) const {
    static IsoCell emptyCell;
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return emptyCell;
    }
    return m_cells[y * m_width + x];
}

bool IsoGrid::IsValid(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) return false;
    return m_cells[y * m_width + x].valid;
}

bool IsoGrid::IsPassable(int x, int y) const {
    if (!IsValid(x, y)) return false;
    return m_cells[y * m_width + x].weight != WEIGHT_IMPASSABLE;
}

float IsoGrid::GetCost(int x, int y) const {
    if (!IsValid(x, y)) return 0.0f;
    return WeightMap::GetWeightCost(m_cells[y * m_width + x].weight);
}

void IsoGrid::ScreenToIso(float screenX, float screenY, int& isoX, int& isoY) const {
    float x = screenX / ISO_HALF_W;
    float y = screenY / ISO_HALF_H;
    isoX = static_cast<int>(floorf((x - y) * 0.5f));
    isoY = static_cast<int>(floorf((x + y) * 0.5f));
}

void IsoGrid::IsoToScreen(int isoX, int isoY, float& screenX, float& screenY) const {
    screenX = (isoX + isoY) * ISO_HALF_W;
    screenY = (isoY - isoX) * ISO_HALF_H;
}

void IsoGrid::WorldToIso(float worldX, float worldY, int& isoX, int& isoY) const {
    float isoXF = (worldX / ISO_HALF_W + worldY / ISO_HALF_H) * 0.5f;
    float isoYF = (worldY / ISO_HALF_H - worldX / ISO_HALF_W) * 0.5f;
    isoX = static_cast<int>(floorf(isoXF));
    isoY = static_cast<int>(floorf(isoYF));
}

void IsoGrid::IsoToWorld(int isoX, int isoY, float& worldX, float& worldY) const {
    worldX = (isoX + isoY) * ISO_HALF_W;
    worldY = (isoY - isoX) * ISO_HALF_H;
}

} // namespace Logic