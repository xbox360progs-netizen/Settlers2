#include "BuildingDefinition.h"

namespace World {

    // Data is a direct transcription of the old switch-based GetBuildingCost table.
    // buildTime is 100 for all types (matches old requiredProgress = 100).

    static const BuildingDefinition g_buildings[] = {
        { BuildingType_None,       { { ResourceType_None, 0, 0, false }, }, 100, PT_None },
        { BuildingType_Woodcutter, { { ResourceType_None, 0, 0, false }, }, 100, PT_Woodcutter },
        { BuildingType_Forester,   { { ResourceType_None, 0, 0, false }, }, 100, PT_Forester },
        { BuildingType_Sawmill,    { { ResourceType_Wood,  2, 0, false }, }, 100, PT_Sawmill },
        { BuildingType_Stonemason, { { ResourceType_Wood,  2, 0, false }, }, 100, PT_Stonemason },
        { BuildingType_Fisher,     { { ResourceType_Wood,  1, 0, false }, }, 100, PT_Fisher },
        { BuildingType_Hunter,     { { ResourceType_Wood,  1, 0, false }, }, 100, PT_Hunter },
        { BuildingType_Farm,       { { ResourceType_Wood,  2, 0, false }, }, 100, PT_Farm },
        { BuildingType_Mill,       { { ResourceType_Wood,  3, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100, PT_Mill },
        { BuildingType_Bakery,     { { ResourceType_Wood,  2, 0, false }, }, 100, PT_Bakery },
        { BuildingType_CoalMine,   { { ResourceType_Wood,  2, 0, false }, }, 100, PT_CoalMine },
        { BuildingType_IronMine,   { { ResourceType_Wood,  2, 0, false }, { ResourceType_Stone, 1, 0, false }, }, 100, PT_IronMine },
        { BuildingType_IronSmelter,{ { ResourceType_Wood,  2, 0, false }, }, 100, PT_IronSmelter },
        { BuildingType_Toolmaker,  { { ResourceType_Wood,  2, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100, PT_Toolmaker },
        { BuildingType_GoldMine,   { { ResourceType_Wood,  2, 0, false }, { ResourceType_Stone, 1, 0, false }, }, 100, PT_GoldMine },
        { BuildingType_WeaponSmith,{ { ResourceType_Wood,  2, 0, false }, { ResourceType_Stone, 1, 0, false }, }, 100, PT_WeaponSmith },
        { BuildingType_Barracks,   { { ResourceType_Wood,  4, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100, PT_Barracks },
        { BuildingType_Storehouse, { { ResourceType_Wood,  4, 0, false }, { ResourceType_Stone, 2, 0, false }, }, 100, PT_None },
        { BuildingType_Residence,  { { ResourceType_Wood,  2, 0, false }, }, 100, PT_None },
        { BuildingType_Well,       { { ResourceType_Stone, 2, 0, false }, }, 100, PT_Well },
    };

    const BuildingDefinition& GetBuildingDefinition(BuildingType type)
    {
        static const int kCount = sizeof(g_buildings) / sizeof(g_buildings[0]);
        int index = static_cast<int>(type);
        if (index < 0 || index >= kCount)
            index = 0;
        return g_buildings[index];
    }

    BuildingType GetBuildingTypeForProduction(ProductionType type)
    {
        static const int kCount = sizeof(g_buildings) / sizeof(g_buildings[0]);
        for (int i = 0; i < kCount; ++i) {
            if (g_buildings[i].production == type) {
                return g_buildings[i].type;
            }
        }
        return BuildingType_None;
    }

}
