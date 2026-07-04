#pragma once
#include "../Core/ProductionTypes.h"
#include "../Core/ResourceTypes.h"

namespace World {

    struct ResourceAmount {
        ResourceType resource;
        int amount;
    };

    struct ProductionDefinition {
        ProductionType type;
        ResourceAmount consumes[4];
        ResourceAmount produces[4];
        int cycleTime;
    };

    const ProductionDefinition& GetProductionDefinition(ProductionType type);

    // Definition Query API
    // Returns the ProductionType that produces the given resource, or PT_None if not found.
    ProductionType GetProducer(ResourceType resource);

}
