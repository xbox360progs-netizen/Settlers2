#pragma once

#include "TileType.h"

namespace World {

// Weight types for pathfinding
enum WeightType
{
    Weight_Deep = 0,
    Weight_Shallow = 1,
    Weight_Land = 2,
    Weight_Block = 3
};

enum ResourceType
{
    ResourceType_None = 0,
    ResourceType_Wood,
    ResourceType_Planks,
    ResourceType_Fish,
    ResourceType_Coal,
    ResourceType_IronOre,
    ResourceType_GoldOre,
    ResourceType_IronBar,
    ResourceType_GoldBar,
    ResourceType_Stone,
    ResourceType_Meat,
    ResourceType_Wheat,
    ResourceType_Flour,
    ResourceType_Bread,
    ResourceType_Water,
    ResourceType_Tools,
    ResourceType_Trap,
    ResourceType_Field,
    ResourceType_RealWood,    // real wood (deposit_real_wood)
    ResourceType_ExoticWood,  // exotic wood (deposit_exotic_wood)
    ResourceType_BronzeOre,
    ResourceType_Marble,
    ResourceType_Granite,
    ResourceType_Titanium,
    ResourceType_Salpeter,
    ResourceType_WildlifeSpawner_Deer,
    ResourceType_WildlifeSpawner_Rabbit,
    ResourceType_WildlifeSpawner_Crocodile,
    ResourceType_WildlifeSpawner_Snake,
    ResourceType_WaterSource,
    ResourceType_Count
};

struct ResourceNode
{
    BYTE weight;
    ResourceType type;
    int amount;
    bool isVisible;

    ResourceNode() : weight(Weight_Land), type(ResourceType_None), amount(0), isVisible(true) {}
    ResourceNode(WeightType w, ResourceType t, int a, bool visible = true)
        : weight(w), type(t), amount(a), isVisible(visible) {}
};

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

inline const char* ResourceTypeToIconName(ResourceType type)
{
    // Simplified for now, map to atlas icon names if needed
    return "";
}

inline int GetDefaultResourceAmount(ResourceType type)
{
    return 10;
}

inline bool IsDepositResource(ResourceType type)
{
    return type != ResourceType_None;
}

inline bool ResourceRequiresMountain(ResourceType type)
{
    return (type == ResourceType_Stone || type == ResourceType_Coal || type == ResourceType_IronOre || type == ResourceType_GoldOre);
}

inline const char* ResourceTypeToBuildingSpriteName(ResourceType type)
{
    return "";
}

inline ResourceType TileTypeToResourceType(TileType tileType)
{
    return ResourceType_None;
}

inline const char* WeightTypeToString(WeightType weight)
{
    switch (weight)
    {
        case Weight_Deep: return "Deep";
        case Weight_Shallow: return "Shallow";
        case Weight_Land: return "Land";
        case Weight_Block: return "Block";
        default: return "Unknown";
    }
}

} // namespace World
