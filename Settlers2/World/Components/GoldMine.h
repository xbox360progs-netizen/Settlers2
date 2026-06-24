#ifndef WORLD_COMPONENTS_GOLDMINE_H
#define WORLD_COMPONENTS_GOLDMINE_H

#include "MineBuilding.h"

namespace World {

class GoldMine : public MineBuilding {
public:
    GoldMine(int x, int y, uint8_t o, Map* m)
        : MineBuilding(BuildingType::GoldMine, x, y, o, m, ResourceType_GoldOre) {}
};

} // namespace World

#endif
