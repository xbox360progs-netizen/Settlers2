#pragma once

#include "../World/Components/Building.h"

namespace World {
    class Map;
    class Flag;
    struct SpriteRegion;
}

namespace Scene {

class ConstructionVisualizer {
public:
    ConstructionVisualizer(World::Map* map);

    void SetupConstructionSiteTiles(World::Flag* flag, int siteX, int siteY, World::BuildingType buildingType);
    void FixConstructionTilesUV();
    void ClearBuildingFootprint(int startX, int startY, int width, int height);

    static const float CONSTRUCTION_U0;
    static const float CONSTRUCTION_V0;
    static const float CONSTRUCTION_U1;
    static const float CONSTRUCTION_V1;
    static const uint32_t CONSTRUCTION_ATLAS_W;
    static const uint32_t CONSTRUCTION_ATLAS_H;
    static const uint32_t CONSTRUCTION_PIXEL_X;
    static const uint32_t CONSTRUCTION_PIXEL_Y;
    static const uint32_t CONSTRUCTION_PIXEL_W;
    static const uint32_t CONSTRUCTION_PIXEL_H;

private:
    World::Map* m_map;
};

} // namespace Scene
