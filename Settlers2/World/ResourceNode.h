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
    ResourceType_Tree,
    ResourceType_Fish,
    ResourceType_Coal,
    ResourceType_Gold,
    ResourceType_Iron,
    ResourceType_Stone,
    ResourceType_Meat
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
        case ResourceType_Tree: return "Tree";
        case ResourceType_Fish: return "Fish";
        case ResourceType_Coal: return "Coal";
        case ResourceType_Gold: return "Gold";
        case ResourceType_Iron: return "Iron";
        case ResourceType_Stone: return "Stone";
        case ResourceType_Meat: return "Meat";
        default: return "Unknown";
    }
}

inline const char* ResourceTypeToIconName(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Tree: return "deposit_wood";
        case ResourceType_Fish: return "deposit_fish";
        case ResourceType_Coal: return "deposit_coal";
        case ResourceType_Gold: return "deposit_gold";
        case ResourceType_Iron: return "deposit_iron";
        case ResourceType_Stone: return "deposit_stone";
        case ResourceType_Meat: return "deposit_meat";
        default: return "";
    }
}

inline int GetDefaultResourceAmount(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Tree: return 10;
        case ResourceType_Fish: return 20;
        case ResourceType_Coal: return 30;
        case ResourceType_Gold: return 5;
        case ResourceType_Iron: return 25;
        case ResourceType_Stone: return 15;
        case ResourceType_Meat: return 15;
        default: return 0;
    }
}

// Returns true if this resource type requires the deposit preview flow (building preview before placing the icon)
inline bool IsDepositResource(ResourceType type)
{
    return type != ResourceType_None && type != ResourceType_Tree;
}

// Maps a deposit resource type to the building sprite name used during preview placement
inline const char* ResourceTypeToBuildingSpriteName(ResourceType type)
{
    switch (type)
    {
        case ResourceType_Stone: return "deposit_stone_framework";
        case ResourceType_Fish: return "deposit_fisher";
        case ResourceType_Coal:
        case ResourceType_Gold:
        case ResourceType_Iron: return "deposit_mine";
        case ResourceType_Meat: return "deposit_mine_hut";
        default: return "";
    }
}

inline ResourceType TileTypeToResourceType(TileType tileType)
{
    switch (tileType)
    {
        case Tree: return ResourceType_Tree;
        case Mountain: return ResourceType_Stone;
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
