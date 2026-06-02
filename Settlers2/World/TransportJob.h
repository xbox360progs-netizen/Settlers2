#pragma once
#include "ResourceNode.h"

namespace World {
    class Flag; // Forward declaration

    struct TransportJob {
        ResourceType resource;
        Flag* source;
        Flag* destination;
        uint32_t priority;
    };
}
