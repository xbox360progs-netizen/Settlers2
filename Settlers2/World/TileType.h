#pragma once

#include <string>

namespace World {

enum TileType
{
	None,
    Tree,
    Mountain,
    MountainOnWater,
    Rock,
    Building,
    Decoration
};

inline const char* TileTypeToString(TileType type)
{
    switch (type)
    {
		case None: return "None";
        case Tree: return "Tree";
        case Mountain: return "Mountain";
        case MountainOnWater: return "MountainOnWater";
        case Rock: return "Rock";
        case Building: return "Building";
        case Decoration: return "Decoration";
        default: return "Unknown";
    }
}

inline TileType StringToTileType(const std::string& str)
{
	if (str == "None") return None;
    if (str == "Tree") return Tree;
    if (str == "Mountain") return Mountain;
    if (str == "MountainOnWater") return MountainOnWater;
    if (str == "Rock") return Rock;
    if (str == "Building") return Building;
    if (str == "Decoration") return Decoration;
    return Tree;
}

} // namespace World
