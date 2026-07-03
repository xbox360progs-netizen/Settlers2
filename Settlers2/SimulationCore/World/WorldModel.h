#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "../Transport/TransportTypes.h"

namespace World {

    static const int kMaxPendingRequests = 128;

    struct TransportRequest {
        ResourceType resource;
        FlagId origin;
        FlagId destination;
        TransportTaskReason reason;
        bool fulfilled;
    };

    struct WorldModel {
        uint32_t width;
        uint32_t height;

        TransportRequest pendingRequests[kMaxPendingRequests];
        int pendingRequestCount;

        WorldModel()
            : width(0)
            , height(0)
            , pendingRequestCount(0)
        {
        }
    };

} // namespace World
