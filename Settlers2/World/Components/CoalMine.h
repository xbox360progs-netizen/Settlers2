#ifndef WORLD_COMPONENTS_COALMINE_H
#define WORLD_COMPONENTS_COALMINE_H

#include "MineBuilding.h"

namespace World {

class CoalMine : public MineBuilding {
public:
    CoalMine(int x, int y, uint8_t o, Map* m)
        : MineBuilding(BuildingType::CoalMine, x, y, o, m, ResourceType_Coal) {}
};

} // namespace World

#endif
