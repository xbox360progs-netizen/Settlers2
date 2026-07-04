#pragma once
#include "../Core/ResourceTypes.h"
#include "../Core/ProductionTypes.h"

namespace World {

    static const int kMaxFoodTypes = 4;

    struct FoodOption {
        ResourceType resource;
        int efficiency;
    };

    struct ConsumptionDefinition {
        ProductionType mineType;
        FoodOption foodOptions[kMaxFoodTypes];
        int baseRate;
        int fedRate;
        int foodPerCycle;
    };

    const ConsumptionDefinition& GetConsumptionDefinition(ProductionType mineType);

    bool IsMine(ProductionType type);

    ProductionType GetConsumptionMineType(BuildingType buildingType);

}
