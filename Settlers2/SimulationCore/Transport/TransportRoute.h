#pragma once
#include <stdint.h>
#include "TransportTypes.h"

namespace World {

    static const int kMaxRouteLength = 64;

    struct TransportRoute {
        uint8_t count;
        FlagId flags[kMaxRouteLength];
    };

} // namespace World
