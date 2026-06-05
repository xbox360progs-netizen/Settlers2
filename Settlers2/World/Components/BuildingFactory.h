#pragma once
#include "Building.h"

namespace World {
    class Map;
    Building* CreateBuilding(BuildingType type, int x, int y, uint8_t owner, Map* map);
}
