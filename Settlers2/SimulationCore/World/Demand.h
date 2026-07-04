#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "../Core/DemandTypes.h"
#include "../Transport/TransportTypes.h"

namespace World {

    struct Demand {
        ResourceType type;
        uint32_t remaining;
        uint32_t totalRequested;
        FlagId targetFlag;
        int priority;
        DemandOwner owner;
        TransportTaskReason reason;

        // PR A — prevent duplicate TransportRequests
        TransportTaskId activeTask;

        Demand()
            : type(ResourceType_None)
            , remaining(0)
            , totalRequested(0)
            , targetFlag(0)
            , priority(0)
            , owner(DemandOwner_Construction)
            , reason(TTR_Construction)
            , activeTask(0)
        {
        }
    };

} // namespace World
