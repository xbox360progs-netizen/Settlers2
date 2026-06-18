#include "stdafx.h"
#include "BuildingFactory.h"
#include "Woodcutter.h"
#include "Forester.h"
#include "Sawmill.h"
#include "Farm.h"
#include "Fisher.h"
#include "Hunter.h"
#include "Stonemason.h"
#include "CoalMine.h"
#include "IronMine.h"
#include "GoldMine.h"
#include "IronSmelter.h"
#include "GoldSmelter.h"
#include "Bakery.h"
#include "Mill.h"
#include "ToolWorkshop.h"

namespace World {

    Building* CreateBuilding(BuildingType type, int x, int y, uint8_t owner, Map* map) {
        switch (type) {
            case Woodcutter:     return new class Woodcutter(x, y, owner, map);
            case Forester:       return new class Forester(x, y, owner, map);
            case Sawmill:        return new class Sawmill(x, y, owner, map);
            case Farm:           return new class Farm(x, y, owner, map);
            case Fisher:         return new class Fisher(x, y, owner, map);
            case Hunter:         return new class Hunter(x, y, owner, map);
            case Stonemason:     return new class Stonemason(x, y, owner, map);
            case CoalMine:       return new class CoalMine(x, y, owner, map);
            case IronMine:       return new class IronMine(x, y, owner, map);
            case GoldMine:       return new class GoldMine(x, y, owner, map);
            case IronSmelter:    return new class IronSmelter(x, y, owner, map);
            case GoldSmelter:    return new class GoldSmelter(x, y, owner, map);
            case Bakery:         return new class Bakery(x, y, owner, map);
            case Mill:           return new class Mill(x, y, owner, map);
            case ToolWorkshop:   return new class ToolWorkshop(x, y, owner, map);
            default:             return NULL;
        }
    }

}
