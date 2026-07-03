#pragma once
#include <stdint.h>
#include "TransportTypes.h"

namespace World {

    static const int kMaxRouteLength = 64;

    struct TransportRoute {
        uint8_t count;                          // number of flags in route (>= 2)
        FlagId flags[kMaxRouteLength];          // ordered: origin, hop1, ..., destination
    };

} // namespace World
