#pragma once
#include "ResourceTypes.h"

namespace World {

inline const char* ResourceTypeToString(ResourceType type)
{
    switch (type)
    {
        case ResourceType_None: return "None";
        case ResourceType_Wood: return "Wood";
        case ResourceType_Planks: return "Planks";
        case ResourceType_Fish: return "Fish";
        case ResourceType_Coal: return "Coal";
        case ResourceType_IronOre: return "IronOre";
        case ResourceType_GoldOre: return "GoldOre";
        case ResourceType_IronBar: return "IronBar";
        case ResourceType_GoldBar: return "GoldBar";
        case ResourceType_Stone: return "Stone";
        case ResourceType_Meat: return "Meat";
        case ResourceType_Wheat: return "Wheat";
        case ResourceType_Flour: return "Flour";
        case ResourceType_Bread: return "Bread";
        case ResourceType_Water: return "Water";
        case ResourceType_Tools: return "Tools";
        case ResourceType_Trap: return "Trap";
        case ResourceType_Field: return "Field";
        case ResourceType_RealWood: return "RealWood";
        case ResourceType_ExoticWood: return "ExoticWood";
        case ResourceType_BronzeOre: return "BronzeOre";
        case ResourceType_BronzeBar: return "BronzeBar";
        case ResourceType_Marble: return "Marble";
        case ResourceType_Granite: return "Granite";
        case ResourceType_Titanium: return "Titanium";
        case ResourceType_Salpeter: return "Salpeter";
        case ResourceType_WildlifeSpawner_Deer: return "DeerSpawner";
        case ResourceType_WildlifeSpawner_Rabbit: return "RabbitSpawner";
        case ResourceType_WildlifeSpawner_Crocodile: return "CrocodileSpawner";
        case ResourceType_WildlifeSpawner_Snake: return "SnakeSpawner";
        case ResourceType_WaterSource: return "WaterSource";
        default: return "Unknown";
    }
}

}
