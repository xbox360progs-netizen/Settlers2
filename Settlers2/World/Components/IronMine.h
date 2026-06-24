#ifndef WORLD_COMPONENTS_IRONMINE_H
#define WORLD_COMPONENTS_IRONMINE_H

#include "MineBuilding.h"

namespace World {

class IronMine : public MineBuilding {
public:
    IronMine(int x, int y, uint8_t o, Map* m)
        : MineBuilding(BuildingType::IronMine, x, y, o, m, ResourceType_IronOre) {}
};

} // namespace World

#endif
