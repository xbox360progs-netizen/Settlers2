#pragma once

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
    ResourceType_Stone
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
        default: return "Unknown";
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
