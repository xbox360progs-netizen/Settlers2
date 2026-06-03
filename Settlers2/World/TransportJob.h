#pragma once
#include "ResourceNode.h"

namespace World {
    class Flag; // Forward declaration

    struct Cargo {
        ResourceType type;
        uint8_t amount;

        Cargo() : type(ResourceType_None), amount(0) {}
        Cargo(ResourceType t, uint8_t a) : type(t), amount(a) {}
    };

    struct TransportJob {
        Cargo cargo;
        Flag* source;
        Flag* destination;
        uint32_t priority;
    };
}
