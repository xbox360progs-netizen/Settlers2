#pragma once
#include <stdint.h>
#include "TransportTypes.h"

// Phase 7 — Route data. Immutable after creation.

namespace World {

    static const int kMaxRouteLength = 64;

    struct TransportRoute {
        uint8_t count;                          // number of flags in route (>= 2)
        FlagId flags[kMaxRouteLength];          // ordered: origin, hop1, ..., destination

        // Invariants:
        //  - count >= 2
        //  - flags[0]           == origin
        //  - flags[count - 1] == destination
        //  - All flags connected by roads (verified at CreateTask time)
        //  - flags[] never modified after creation
    };

} // namespace World
