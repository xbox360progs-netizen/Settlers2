#pragma once
#include <stdint.h>

// Phase 7 — Priority data. No logic.

namespace World {

    struct TransportPriority {
        uint8_t classPriority;      // set once at creation (based on TaskReason)
        uint8_t dynamicPriority;    // increases over time (anti-starvation)

        // DynamicPriority formula:
        //   dynamicPriority = (currentTick - createdTick) / kPriorityAgeStep
        //   kPriorityAgeStep = 1800 ticks (30 seconds at 60 Hz)
        // Read at selection time; no per-frame computation.
    };

} // namespace World
