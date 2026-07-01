#ifndef WORLD_COMPONENTS_BUILDING_DEFINITION_H
#define WORLD_COMPONENTS_BUILDING_DEFINITION_H

#include "Building.h"
#include "../../UI/UiMessageId.h"

namespace World {

struct BuildingDefinition {
    const char* buildingSprite;
    const char* menuIcon;
    int woodCost;
    int stoneCost;
    int sizeX, sizeY;
    UI::UiMessageId labelId;
    int maxPopulation;
};

enum { BUILDING_TYPE_COUNT = 30 };

extern const BuildingDefinition g_buildingDefinitions[BUILDING_TYPE_COUNT];

} // namespace World

#endif
