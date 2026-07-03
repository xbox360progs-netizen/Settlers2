#include "BuildingDefinition.h"

namespace World {

    // Data is a direct transcription of the old switch-based GetBuildingCost table.
    // buildTime is 100 for all types (matches old requiredProgress = 100).

    static const BuildingDefinition g_buildings[] = {
        { BuildingType_None,       { { ResourceType_None, 0, 0, false }, }, 100 },
        { BuildingType_Woodcutter, { { ResourceType_None, 0, 0, false }, }, 100 },
        { BuildingType_Forester,   { { ResourceType_None, 0, 0, false }, }, 100 },
        { BuildingType_Sawmill,    { { ResourceType_Wood,  2, 0, false }, }, 100 },
        { BuildingType_Stonemason, { { ResourceType_Stone, 2, 0, false }, }, 100 },
        { BuildingType_Fisher,     { { ResourceType_Wood,  1, 0, false }, }, 100 },
        { BuildingType_Hunter,     { { ResourceType_Wood,  1, 0, false }, }, 100 },
        { BuildingType_Farm,       { { ResourceType_Wood,  2, 0, false }, }, 100 },
        { BuildingType_Mill,       { { ResourceType_Wood,  3, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100 },
        { BuildingType_Bakery,     { { ResourceType_Wood,  2, 0, false }, }, 100 },
        { BuildingType_CoalMine,   { { ResourceType_Wood,  2, 0, false }, }, 100 },
        { BuildingType_IronMine,   { { ResourceType_Wood,  2, 0, false }, { ResourceType_Stone, 1, 0, false }, }, 100 },
        { BuildingType_IronSmelter,{ { ResourceType_Wood,  2, 0, false }, }, 100 },
        { BuildingType_Toolmaker,  { { ResourceType_Wood,  2, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100 },
        { BuildingType_Storehouse, { { ResourceType_Wood,  4, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100 },
        { BuildingType_Residence,  { { ResourceType_Wood,  2, 0, false }, }, 100 },
        { BuildingType_Well,       { { ResourceType_Stone, 2, 0, false }, }, 100 },
    };

    const BuildingDefinition& GetBuildingDefinition(BuildingType type)
    {
        static const int kCount = sizeof(g_buildings) / sizeof(g_buildings[0]);
        int index = static_cast<int>(type);
        if (index < 0 || index >= kCount)
            index = 0;
        return g_buildings[index];
    }

}
