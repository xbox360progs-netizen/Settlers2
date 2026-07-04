#pragma once
#include <stdint.h>
#include "../Core/BuildingTypes.h"
#include "../Core/ResourceTypes.h"

namespace World {

    enum PopulationModel {
        PM_StagedTrees,
        PM_SimplePopulation
    };

    struct RenewableResourceDefinition {
        BuildingType buildingType;
        ResourceType harvestedResource;
        uint32_t regenerationRate;
        uint32_t capacity;
        PopulationModel model;
    };

    const RenewableResourceDefinition& GetRenewableResourceDefinition(BuildingType type);

    bool HasRenewableResource(BuildingType type);

}
