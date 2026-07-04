#include "RenewableResourceDefinition.h"

namespace World {

    static const RenewableResourceDefinition g_definitions[] = {
        { BuildingType_Woodcutter, ResourceType_Wood, 0, 0, PM_StagedTrees },
        { BuildingType_Forester,   ResourceType_None, 0, 0, PM_StagedTrees },
        { BuildingType_Hunter,     ResourceType_Meat, 8, 50, PM_SimplePopulation },
        { BuildingType_Fisher,     ResourceType_Fish, 8, 50, PM_SimplePopulation },
    };

    static const int kCount = sizeof(g_definitions) / sizeof(g_definitions[0]);

    const RenewableResourceDefinition& GetRenewableResourceDefinition(BuildingType type)
    {
        for (int i = 0; i < kCount; ++i) {
            if (g_definitions[i].buildingType == type)
                return g_definitions[i];
        }
        return g_definitions[0];
    }

    bool HasRenewableResource(BuildingType type)
    {
        return GetRenewableResourceDefinition(type).buildingType == type;
    }

}
