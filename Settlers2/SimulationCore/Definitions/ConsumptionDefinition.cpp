#include "ConsumptionDefinition.h"
#include "../Core/BuildingTypes.h"

namespace World {

    static const ConsumptionDefinition g_consumptions[] = {
        { PT_CoalMine,
            { { ResourceType_Meat, 1 },
              { ResourceType_Fish, 1 },
              { ResourceType_Bread, 3 },
              { ResourceType_None, 0 } },
            0, 30, 1 },
        { PT_IronMine,
            { { ResourceType_Meat, 1 },
              { ResourceType_Fish, 1 },
              { ResourceType_Bread, 3 },
              { ResourceType_None, 0 } },
            0, 30, 1 },
        { PT_GoldMine,
            { { ResourceType_Meat, 1 },
              { ResourceType_Fish, 1 },
              { ResourceType_Bread, 3 },
              { ResourceType_None, 0 } },
            0, 30, 1 },
    };

    static const int kConsumptionCount = sizeof(g_consumptions) / sizeof(g_consumptions[0]);

    const ConsumptionDefinition& GetConsumptionDefinition(ProductionType mineType)
    {
        int index = static_cast<int>(mineType);
        for (int i = 0; i < kConsumptionCount; ++i) {
            if (g_consumptions[i].mineType == mineType)
                return g_consumptions[i];
        }
        return g_consumptions[0];
    }

    bool IsMine(ProductionType type)
    {
        for (int i = 0; i < kConsumptionCount; ++i) {
            if (g_consumptions[i].mineType == type)
                return true;
        }
        return false;
    }

    ProductionType GetConsumptionMineType(BuildingType buildingType)
    {
        switch (buildingType) {
            case BuildingType_CoalMine: return PT_CoalMine;
            case BuildingType_IronMine: return PT_IronMine;
            case BuildingType_GoldMine: return PT_GoldMine;
            default: return PT_None;
        }
    }

}
