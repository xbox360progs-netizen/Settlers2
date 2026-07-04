#pragma once
#include "../Core/ResourceTypes.h"

namespace World {

    struct ResourceDefinition {
        ResourceType type;
        const char* name;
        int maxStack;
    };

    const ResourceDefinition& GetResourceDefinition(ResourceType type);

}
