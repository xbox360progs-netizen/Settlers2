#ifndef WORLD_COMPONENTS_BRONZEMINE_H
#define WORLD_COMPONENTS_BRONZEMINE_H

#include "MineBuilding.h"

namespace World {

class BronzeMine : public MineBuilding {
public:
    BronzeMine(int x, int y, uint8_t o, Map* m)
        : MineBuilding(BuildingType::BronzeMine, x, y, o, m, ResourceType_BronzeOre) {}
};

} // namespace World

#endif
