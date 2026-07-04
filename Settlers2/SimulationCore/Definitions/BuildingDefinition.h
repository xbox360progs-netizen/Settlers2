#pragma once
#include "../Core/BuildingTypes.h"
#include "../Core/ResourceTypes.h"
#include "../Core/ProductionTypes.h"
#include "../Construction/ConstructionSite.h"

namespace World {

    struct BuildingDefinition {
        BuildingType type;
        BuildResourceSlot buildCost[4];
        int buildTime;
        ProductionType production;
    };

    const BuildingDefinition& GetBuildingDefinition(BuildingType type);

    // Returns the BuildingType associated with a given ProductionType, or BuildingType_None.
    BuildingType GetBuildingTypeForProduction(ProductionType type);

}
