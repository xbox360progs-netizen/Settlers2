#include "ProductionDefinition.h"

namespace World {

    static const ProductionDefinition g_productions[] = {
        { PT_None,        { { ResourceType_None, 0 }, }, { { ResourceType_None, 0 }, }, 0 },
        { PT_Woodcutter,  { { ResourceType_None, 0 }, }, { { ResourceType_Wood, 1 }, }, 30 },
        { PT_Forester,    { { ResourceType_None, 0 }, }, { { ResourceType_None, 0 }, }, 30 },
        { PT_Sawmill,     { { ResourceType_Wood,  2 }, }, { { ResourceType_Planks, 1 }, }, 30 },
        { PT_Stonemason,  { { ResourceType_None, 0 }, }, { { ResourceType_Stone, 1 }, }, 30 },
        { PT_Fisher,      { { ResourceType_None, 0 }, }, { { ResourceType_Fish, 1 }, }, 30 },
        { PT_Hunter,      { { ResourceType_None, 0 }, }, { { ResourceType_Meat, 1 }, }, 30 },
        { PT_Farm,        { { ResourceType_None, 0 }, }, { { ResourceType_Wheat, 1 }, }, 30 },
        { PT_Mill,        { { ResourceType_Wheat, 1 }, }, { { ResourceType_Flour, 1 }, }, 30 },
        { PT_Bakery,      { { ResourceType_Flour, 1 }, }, { { ResourceType_Bread, 1 }, }, 30 },
        { PT_CoalMine,    { { ResourceType_None, 0 }, }, { { ResourceType_Coal, 1 }, }, 30 },
        { PT_IronMine,    { { ResourceType_None, 0 }, }, { { ResourceType_IronOre, 1 }, }, 30 },
        { PT_GoldMine,    { { ResourceType_None, 0 }, }, { { ResourceType_GoldOre, 1 }, }, 30 },
        { PT_IronSmelter, { { ResourceType_IronOre, 1 }, }, { { ResourceType_IronBar, 1 }, }, 30 },
        { PT_Toolmaker,   { { ResourceType_Wood,  1 }, { ResourceType_Stone, 1 }, }, { { ResourceType_Tools, 1 }, }, 30 },
        { PT_WeaponSmith, { { ResourceType_IronBar, 1 }, { ResourceType_Coal, 1 }, }, { { ResourceType_Weapons, 1 }, }, 30 },
        { PT_Barracks,    { { ResourceType_Weapons, 1 }, }, { { ResourceType_Soldiers, 1 }, }, 50 },
        { PT_Well,        { { ResourceType_None, 0 }, }, { { ResourceType_Water, 1 }, }, 30 },
    };

    const ProductionDefinition& GetProductionDefinition(ProductionType type)
    {
        static const int kCount = sizeof(g_productions) / sizeof(g_productions[0]);
        int index = static_cast<int>(type);
        if (index < 0 || index >= kCount)
            index = 0;
        return g_productions[index];
    }

    ProductionType GetProducer(ResourceType resource)
    {
        static const int kCount = sizeof(g_productions) / sizeof(g_productions[0]);
        for (int i = 0; i < kCount; ++i) {
            for (int p = 0; p < 4; ++p) {
                if (g_productions[i].produces[p].resource == resource && g_productions[i].produces[p].amount > 0) {
                    return g_productions[i].type;
                }
            }
        }
        return PT_None;
    }

}
