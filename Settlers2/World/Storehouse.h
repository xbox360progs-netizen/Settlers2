#pragma once
#include "ResourceNode.h"

namespace World {

static const int MAX_STOREHOUSES = 32;

struct Storehouse {
    uint32_t resources[ResourceType_Count];

    Storehouse() {
        for (int i = 0; i < ResourceType_Count; ++i)
            resources[i] = 0;
    }
};

}
