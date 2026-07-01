#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "TransportTypes.h"
#include "TransportRoute.h"
#include "TransportPriority.h"

// Phase 7 — Task data. Single source of truth for one shipment.
// One TransportTask = one physical unit. No amount field.
// No logic. Only Controller may modify fields.

namespace World {

    class Cargo;
    class Carrier;

    struct TransportTask {
        TransportTaskId id;             // unique, monotonic

        ResourceType resource;          // one task = one physical unit

        TransportTaskState state;

        TransportTaskReason reason;
        TransportPriority priority;

        TransportRoute route;           // immutable after creation
        uint8_t hopIndex;               // current position: route.flags[hopIndex]
        FlagId targetFlag;              // next flag the assigned carrier walks to

        Cargo* cargo;                   // NULL unless cargo exists
        Carrier* carrier;               // NULL unless assigned/moving

        uint32_t createdTick;           // tick at creation (anti-starvation)

        // Queue linkage (used by Controller for per-flag waiting lists)
        TransportTask* nextWaiting;     // next task in same waiting queue (NULL if tail)

        // Invariants:
        //  - route immutable after creation
        //  - hopIndex < route.count
        //  - state == WaitingAtSource / ArrivedAtHop ⇒ carrier == NULL
        //  - state == Moving ⇒ carrier != NULL  AND  cargo != NULL
        //  - state == Delivered ⇒ carrier == NULL  AND  cargo == NULL
        //  - id never reused (monotonic)
    };

} // namespace World
