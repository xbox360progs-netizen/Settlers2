#pragma once

namespace World {

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
    ResourceType type;
    int amount;
    bool isVisible;

    ResourceNode()
        : type(ResourceType_None)
        , amount(0)
        , isVisible(true)
    {
    }

    ResourceNode(ResourceType t, int a, bool visible = true)
        : type(t)
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

} // namespace World
