#pragma once
#include "../Core/BuildingTypes.h"
#include "../Core/ResourceTypes.h"
#include "../Construction/ConstructionSite.h"

namespace World {

    struct BuildingDefinition {
        BuildingType type;
        BuildResourceSlot buildCost[4];
        int buildTime;
    };

    const BuildingDefinition& GetBuildingDefinition(BuildingType type);

}
