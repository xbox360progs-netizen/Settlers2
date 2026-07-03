#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "TransportTypes.h"
#include "TransportRoute.h"

namespace World {

    struct Cargo;
    class Carrier;

    struct TransportTask {
        TransportTaskId id;

        ResourceType resource;

        TransportTaskState state;

        TransportTaskReason reason;
        uint16_t basePriority;
        uint16_t enqueueOrder;

        TransportRoute route;
        uint8_t hopIndex;
        FlagId targetFlag;

        Cargo* cargo;
        Carrier* carrier;

        uint32_t createdTick;
        uint8_t transitionCount;

        uint32_t observerTicketId;

        TransportTask* nextWaiting;
    };

} // namespace World
