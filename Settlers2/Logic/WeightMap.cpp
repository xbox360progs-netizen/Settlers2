#include "stdafx.h"
#include "WeightMap.h"

namespace Logic {

WeightMap::WeightMap()
    : m_width(0), m_height(0) {
}

WeightMap::WeightMap(int width, int height)
    : m_width(width), m_height(height) {
    m_weights.resize(width * height, WEIGHT_DEFAULT);
}

WeightMap::~WeightMap() {
}

void WeightMap::Resize(int width, int height) {
    m_width = width;
    m_height = height;
    m_weights.resize(width * height, WEIGHT_DEFAULT);
}

TileWeight WeightMap::GetWeight(int x, int y) const {
    if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
        return WEIGHT_IMPASSABLE;
    }
    return m_weights[y * m_width + x];
}

void WeightMap::SetWeight(int x, int y, TileWeight weight) {
    if (x >= 0 && x < m_width && y >= 0 && y < m_height) {
        m_weights[y * m_width + x] = weight;
    }
}

bool WeightMap::IsPassable(int x, int y) const {
    return GetWeight(x, y) > WEIGHT_IMPASSABLE;
}

float WeightMap::GetMovementCost(int x, int y) const {
    TileWeight weight = GetWeight(x, y);
    return GetWeightCost(weight);
}

void WeightMap::Clear() {
    std::fill(m_weights.begin(), m_weights.end(), WEIGHT_DEFAULT);
}

TileWeight WeightMap::GetDefaultWeight(World::TileType type) {
    switch (type) {
        case World::Mountain:
        case World::MountainOnWater:
            return WEIGHT_IMPASSABLE;
        case World::Tree:
        case World::Rock:
        case World::Decoration:
            return WEIGHT_LAND; 
        case World::Building:
            return WEIGHT_IMPASSABLE;
        default:
            return WEIGHT_DEFAULT;
    }
}

float WeightMap::GetWeightCost(TileWeight weight) {
    switch (weight) {
        case WEIGHT_IMPASSABLE: return 0.0f;
        case WEIGHT_DEEP_WATER: return 10.0f;
        case WEIGHT_SHALLOW_WATER: return 5.0f;
        case WEIGHT_COAST: return 2.0f;
        case WEIGHT_LAND: return 1.0f;
        case WEIGHT_ROAD: return 0.5f;
        default: return 1.0f;
    }
}

} // namespace Logic