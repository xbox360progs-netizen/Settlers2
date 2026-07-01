#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "TransportTypes.h"
#include "TransportRoute.h"
// TransportPriority.h merged into TransportTypes.h (Phase 7.4)

// Phase 7 — Task data. Single source of truth for one shipment.
// One TransportTask = one physical unit. No amount field.
// No logic. Only Controller may modify fields.

namespace World {

    struct Cargo;
    class Carrier;

    struct TransportTask {
        TransportTaskId id;             // unique, monotonic

        ResourceType resource;          // one task = one physical unit

        TransportTaskState state;

        TransportTaskReason reason;
        uint16_t basePriority;          // set at creation from reason (immutable)
        uint16_t enqueueOrder;          // monotonic, set on EnqueueWaiting (FIFO tiebreak)

        TransportRoute route;           // immutable after creation
        uint8_t hopIndex;               // current position: route.flags[hopIndex]
        FlagId targetFlag;              // next flag the assigned carrier walks to

        Cargo* cargo;                   // NULL unless cargo exists
        Carrier* carrier;               // NULL unless assigned/moving

        uint32_t createdTick;           // tick at creation (age bonus: computed on selection)
        uint8_t transitionCount;        // total state transitions (safety: assert < 64)

        // Queue linkage (used by Controller for per-flag waiting lists)
        TransportTask* nextWaiting;     // next task in same waiting queue (NULL if tail)

        // Invariants:
        //  - route immutable after creation
        //  - hopIndex < route.count
        //  - state == WaitingAtSource / Blocked ⇒ carrier == NULL
        //  - state == Assigned / Moving ⇒ carrier != NULL
        //  - state == Moving ⇒ cargo != NULL
        //  - state == Delivered / Cancelled ⇒ carrier == NULL  AND  cargo == NULL
        //  - id never reused (monotonic)
        //  - transitionCount < 64 (prevents infinite state loops)
    };

} // namespace World
