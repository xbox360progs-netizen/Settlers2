#include "stdafx.h"
#include "ConstructionSite.h"

namespace World {

    // Default resource requirements per building type
    static void GetResourceRequirements(BuildingType type, int& wood, int& stone) {
        switch (type) {
            case Woodcutter:    wood = 3;  stone = 0; break;
            case Sawmill:       wood = 6;  stone = 0; break;
            case CoalMine:      wood = 4;  stone = 0; break;
            case IronMine:      wood = 4;  stone = 0; break;
            case GoldMine:      wood = 4;  stone = 0; break;
            case IronSmelter:   wood = 3;  stone = 3; break;
            case GoldSmelter:   wood = 3;  stone = 3; break;
            case Farm:          wood = 4;  stone = 0; break;
            case Mill:          wood = 4;  stone = 2; break;
            case Bakery:        wood = 3;  stone = 2; break;
            case Fisher:        wood = 3;  stone = 0; break;
            case Hunter:        wood = 3;  stone = 0; break;
            case ToolWorkshop:  wood = 4;  stone = 3; break;
            default:            wood = 2;  stone = 0; break;
        }
    }

    ConstructionSite::ConstructionSite(int x, int y, BuildingType type, Flag* flag)
        : x(x), y(y), buildingType(type), flag(flag)
        , woodNeeded(0), stoneNeeded(0)
        , woodDelivered(0), stoneDelivered(0)
        , woodRequested(0), stoneRequested(0)
        , progress(0.0f)
    {
        GetResourceRequirements(type, woodNeeded, stoneNeeded);
    }

}
