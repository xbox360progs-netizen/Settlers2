#include "ResourceDefinition.h"

namespace World {

    static const ResourceDefinition g_resources[] = {
        { ResourceType_None,                   "None",        0 },
        { ResourceType_Wood,                   "Wood",       32 },
        { ResourceType_Planks,                 "Planks",     32 },
        { ResourceType_Fish,                   "Fish",       32 },
        { ResourceType_Coal,                   "Coal",       32 },
        { ResourceType_IronOre,                "IronOre",    32 },
        { ResourceType_GoldOre,                "GoldOre",    32 },
        { ResourceType_IronBar,                "IronBar",    32 },
        { ResourceType_GoldBar,                "GoldBar",    32 },
        { ResourceType_Stone,                  "Stone",      32 },
        { ResourceType_Meat,                   "Meat",       32 },
        { ResourceType_Wheat,                  "Wheat",      32 },
        { ResourceType_Flour,                  "Flour",      32 },
        { ResourceType_Bread,                  "Bread",      32 },
        { ResourceType_Water,                  "Water",      32 },
        { ResourceType_Tools,                  "Tools",      32 },
        { ResourceType_Trap,                   "Trap",       32 },
        { ResourceType_Field,                  "Field",       1 },
        { ResourceType_RealWood,               "RealWood",   32 },
        { ResourceType_ExoticWood,             "ExoticWood", 32 },
        { ResourceType_BronzeOre,              "BronzeOre",  32 },
        { ResourceType_Marble,                 "Marble",     32 },
        { ResourceType_Granite,                "Granite",    32 },
        { ResourceType_Titanium,               "Titanium",   32 },
        { ResourceType_Salpeter,               "Salpeter",   32 },
        { ResourceType_WildlifeSpawner_Deer,   "DeerSpawner", 1 },
        { ResourceType_WildlifeSpawner_Rabbit, "RabbitSpawner", 1 },
        { ResourceType_WildlifeSpawner_Crocodile, "CrocodileSpawner", 1 },
        { ResourceType_WildlifeSpawner_Snake,  "SnakeSpawner", 1 },
        { ResourceType_WaterSource,            "WaterSource", 1 },
        { ResourceType_BronzeBar,              "BronzeBar",  32 },
    };

    const ResourceDefinition& GetResourceDefinition(ResourceType type)
    {
        static const int kCount = sizeof(g_resources) / sizeof(g_resources[0]);
        int index = static_cast<int>(type);
        if (index < 0 || index >= kCount)
            index = 0;
        return g_resources[index];
    }

}
