#pragma once

#include "TileType.h"

namespace World {

// Weight types for pathfinding (matches Logic::TileWeight)
enum WeightType
{
    Weight_Deep = 0,    // Deep water (impassable or high cost)
    Weight_Shallow = 1, // Shallow water
    Weight_Land = 2,    // Land (default)
    Weight_Block = 3    // Blocked/impassable
};

// Resource types that can be placed on the map
enum ResourceType
{
    ResourceType_None = 0,
    ResourceType_Wood,        // basic wood (was ResourceType_Tree)
    ResourceType_Fish,
    ResourceType_Coal,
    ResourceType_Gold,
    ResourceType_Iron,
    ResourceType_Stone,
    ResourceType_Meat,
    ResourceType_RealWood,    // real wood (deposit_real_wood)
    ResourceType_ExoticWood,  // exotic wood (deposit_exotic_wood)
    ResourceType_Corn,
    ResourceType_Water,
    ResourceType_BronzeOre,
    ResourceType_Marble,
    ResourceType_Granite,
    ResourceType_Titanium,
    ResourceType_Salpeter,
    ResourceType_Count
};

// Resource node attached to a grid position
struct ResourceNode
{
    BYTE weight;          // Pathfinding weight (0-3)
    ResourceType type;    // Resource type
    int amount;           // Resource amount
    bool isVisible;       // Visibility in game mode

    ResourceNode()
        : weight(Weight_Land)
        , type(ResourceType_None)
        , amount(0)
        , isVisible(true)
    {
    }

    ResourceNode(WeightType w, ResourceType t, int a, bool visible = true)
        : weight(w)
        , type(t)
        , amount(a)
        , isVisible(visible)
    {
    }
};

inline const char* ResourceTypeToString(ResourceType type)
{
    switch (type)
    {
        case ResourceType_None: return "None";
        case ResourceType_Wood: return "Wood";
        case ResourceType_Fish: return "Fish";
        case ResourceType_Coal: return "Coal";
        case ResourceType_Gold: return "Gold";
        case ResourceType_Iron: return "Iron";
        case ResourceType_Stone: return "Stone";
        case ResourceType_Meat: return "Meat";
        case ResourceType_RealWood: return "RealWood";
        case ResourceType_ExoticWood: return "ExoticWood";
        case ResourceType_Corn: return "Corn";
        case ResourceType_Water: return "Water";
        case ResourceType_BronzeOre: return "BronzeOre";
        case ResourceType_Marble: return "Marble";
        case ResourceType_Granite: return "Granite";
        case ResourceType_Titanium: return "Titanium";
        case ResourceType_Salpeter: return "Salpeter";
        default: return "Unknown";
    }
}

inline const char* ResourceTypeToIconName(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Wood: return "deposit_wood";
        case ResourceType_Fish: return "deposit_fish";
        case ResourceType_Coal: return "deposit_coal";
        case ResourceType_Gold: return "deposit_gold";
        case ResourceType_Iron: return "deposit_iron";
        case ResourceType_Stone: return "deposit_stone";
        case ResourceType_Meat: return "deposit_meat";
        case ResourceType_RealWood: return "deposit_real_wood";
        case ResourceType_ExoticWood: return "deposit_exotic_wood";
        case ResourceType_Corn: return "deposit_corn";
        case ResourceType_Water: return "deposit_water";
        case ResourceType_BronzeOre: return "deposit_bronze_ore";
        case ResourceType_Marble: return "deposit_marble";
        case ResourceType_Granite: return "deposit_granite";
        case ResourceType_Titanium: return "deposit_titanium";
        case ResourceType_Salpeter: return "deposit_salpeter";
        default: return "";
    }
}

inline int GetDefaultResourceAmount(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Wood: return 10;
        case ResourceType_Fish: return 20;
        case ResourceType_Coal: return 30;
        case ResourceType_Gold: return 5;
        case ResourceType_Iron: return 25;
        case ResourceType_Stone: return 15;
        case ResourceType_Meat: return 15;
        case ResourceType_RealWood: return 15;
        case ResourceType_ExoticWood: return 8;
        case ResourceType_Corn: return 20;
        case ResourceType_Water: return 20;
        case ResourceType_BronzeOre: return 20;
        case ResourceType_Marble: return 15;
        case ResourceType_Granite: return 15;
        case ResourceType_Titanium: return 10;
        case ResourceType_Salpeter: return 10;
        default: return 0;
    }
}

// Returns true if this resource type requires the deposit preview flow (building preview before placing the icon)
inline bool IsDepositResource(ResourceType type)
{
    return type != ResourceType_None;
}

inline bool ResourceRequiresMountain(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Stone:
        case ResourceType_Coal:
        case ResourceType_Gold:
        case ResourceType_Iron:
        case ResourceType_BronzeOre:
        case ResourceType_Marble:
        case ResourceType_Granite:
        case ResourceType_Titanium:
        case ResourceType_Salpeter:
            return true;
        default:
            return false;
    }
}

// Maps a deposit resource type to the building sprite name used during preview placement
inline const char* ResourceTypeToBuildingSpriteName(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Stone:
        case ResourceType_Marble:
        case ResourceType_Granite: return "deposit_stone_framework";
        case ResourceType_Fish: return "deposit_fisher";
        case ResourceType_Coal:
        case ResourceType_Gold:
        case ResourceType_Iron:
        case ResourceType_BronzeOre:
        case ResourceType_Titanium:
        case ResourceType_Salpeter: return "deposit_mine";
        case ResourceType_Meat: return "deposit_mine_hut";
        case ResourceType_Corn:
        case ResourceType_Water: return "deposit_mine_hut";
        default: return "";
    }
}

inline ResourceType TileTypeToResourceType(TileType tileType)
{
    switch (tileType)
    {
        case Tree: return ResourceType_Wood;
        case MountainOnWater: return ResourceType_Fish;
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
