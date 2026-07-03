#pragma once

#include <cstdint>
#include "../World/TileType.h"
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Core/ResourceDebug.h"

namespace World {

// Weight types for pathfinding
enum WeightType
{
    Weight_Deep = 0,
    Weight_Shallow = 1,
    Weight_Land = 2,
    Weight_Block = 3
};

// Tree growth states for ResourceType_Wood world nodes
// Stored in ResourceNode.amount
enum TreeState : int {
    TreeState_Empty   = 0,
    TreeState_Sapling = 1,
    TreeState_Young   = 2,
    TreeState_Mature  = 3,
    TreeState_Stump   = 4   // After cutting, before decaying to Empty
};

inline bool IsTree(ResourceType type) { return type == ResourceType_Wood; }
// Mature if type=Wood and amount is a living tree state (not Empty, not Stump)
// Mature if amount >= Mature(3) and not Stump(4)
inline bool IsTreeMature(int amount) { return amount >= TreeState_Mature && amount != TreeState_Stump; }
// Alive if >= Sapling(1) and not Stump(4)
inline bool IsTreeAlive(int amount) { return amount >= TreeState_Sapling && amount != TreeState_Stump; }
inline bool IsTreeStump(int amount) { return amount == TreeState_Stump; }

struct ResourceNode
{
    uint8_t weight;
    ResourceType type;
    int amount;
    bool isVisible;
    bool surveyed;   // true after geologist surveys this mountain

    ResourceNode() : weight(Weight_Land), type(ResourceType_None), amount(0), isVisible(true), surveyed(false) {}
    ResourceNode(WeightType w, ResourceType t, int a, bool visible = true)
        : weight(w), type(t), amount(a), isVisible(visible), surveyed(false) {}
};

inline const char* ResourceTypeToIconName(ResourceType type)
{
    switch (type) {
        case ResourceType_Wood:       return "r_wood";
        case ResourceType_Stone:      return "r_stone";
        case ResourceType_Planks:     return "r_planks";
        case ResourceType_Fish:       return "r_fish";
        case ResourceType_Meat:       return "r_meat";
        case ResourceType_Bread:      return "r_bread";
        case ResourceType_Coal:       return "r_coal";
        case ResourceType_IronOre:    return "r_ironore";
        case ResourceType_GoldOre:    return "r_goldore";
        case ResourceType_IronBar:    return "r_ironbar";
        case ResourceType_GoldBar:    return "r_goldbar";
        case ResourceType_Tools:      return "r_tools";
        case ResourceType_Wheat:      return "r_wheat";
        case ResourceType_Flour:      return "r_flour";
        case ResourceType_Water:      return "r_water";
        case ResourceType_BronzeBar: return "r_bronzebar";
        case ResourceType_WildlifeSpawner_Deer: return "r_deer";
        case ResourceType_WildlifeSpawner_Rabbit: return "r_rabbit";
        case ResourceType_WildlifeSpawner_Crocodile: return "r_crocodile";
        case ResourceType_WildlifeSpawner_Snake: return "r_snake";
        default: return "";
    }
}

inline const char* ResourceTypeToDepositIconName(ResourceType type)
{
    switch (type) {
        case ResourceType_Wood:       return "deposit_wood";
        case ResourceType_Planks:     return "deposit_planks";
        case ResourceType_Fish:       return "deposit_fish";
        case ResourceType_Meat:       return "deposit_meat";
        case ResourceType_Bread:      return "deposit_bread";
        case ResourceType_Coal:       return "deposit_coal";
        case ResourceType_IronOre:    return "deposit_iron";
        case ResourceType_GoldOre:    return "deposit_gold";
        case ResourceType_IronBar:    return "deposit_iron_bar";
        case ResourceType_GoldBar:    return "deposit_gold_bar";
        case ResourceType_Stone:      return "deposit_stone";
        case ResourceType_Tools:      return "deposit_tools";
        case ResourceType_Wheat:      return "deposit_corn";
        case ResourceType_Flour:      return "deposit_flour";
        case ResourceType_Water:      return "deposit_water";
        case ResourceType_WildlifeSpawner_Deer: return "deposit_deer";
        case ResourceType_WildlifeSpawner_Rabbit: return "deposit_rabbit";
        case ResourceType_WildlifeSpawner_Crocodile: return "deposit_crocodile";
        case ResourceType_WildlifeSpawner_Snake: return "deposit_snake";
        case ResourceType_RealWood:   return "deposit_real_wood";
        case ResourceType_ExoticWood: return "deposit_exotic_wood";
        case ResourceType_BronzeOre:  return "deposit_bronze_ore";
        case ResourceType_BronzeBar:  return "deposit_bronze_bar";
        case ResourceType_Marble:     return "deposit_marble";
        case ResourceType_Granite:    return "deposit_granite";
        case ResourceType_Titanium:   return "deposit_titanium";
        case ResourceType_Salpeter:   return "deposit_salpeter";
        case ResourceType_Trap:       return "deposit_trap";
        default: return "";
    }
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
    return (type == ResourceType_Stone || type == ResourceType_Coal || type == ResourceType_IronOre || type == ResourceType_GoldOre || type == ResourceType_BronzeOre);
}

inline const char* ResourceTypeToBuildingSpriteName(ResourceType type)
{
    return "";
}

inline ResourceType TileTypeToResourceType(TileType tileType)
{
    switch (tileType) {
        case Tree: return ResourceType_Wood;
        case Mountain: return ResourceType_Stone;
        case MountainOnWater: return ResourceType_Stone;
        case Rock: return ResourceType_Stone;
        default: return ResourceType_None;
    }
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
