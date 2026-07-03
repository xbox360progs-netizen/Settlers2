#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "../Transport/TransportTypes.h"
#include "../Construction/ConstructionRequest.h"
#include "../Construction/ConstructionSite.h"

namespace World {

    static const int kMaxPendingRequests = 128;
    static const int kMaxConstructionRequests = 128;
    static const int kMaxConstructionSites = 64;
    static const int kMaxDeliveryEvents = 64;

    struct TransportRequest {
        ResourceType resource;
        FlagId origin;
        FlagId destination;
        TransportTaskReason reason;
        bool fulfilled;
    };

    enum DeliveryEventType {
        DET_Completed
    };

    struct DeliveryEvent {
        DeliveryEventType type;
        ResourceType resource;
        int amount;
        FlagId destinationFlag;
        TransportTaskReason reason;

        DeliveryEvent()
            : type(DET_Completed)
            , resource(ResourceType_None)
            , amount(0)
            , destinationFlag(0)
            , reason(TTR_Construction)
        {
        }
    };

    struct WorldModel {
        uint32_t width;
        uint32_t height;

        TransportRequest pendingRequests[kMaxPendingRequests];
        int pendingRequestCount;

        ConstructionRequest pendingConstructionRequests[kMaxConstructionRequests];
        int pendingConstructionCount;

        ConstructionSite activeSites[kMaxConstructionSites];
        int activeSiteCount;

        DeliveryEvent deliveryEvents[kMaxDeliveryEvents];
        int deliveryEventCount;

        WorldModel()
            : width(0)
            , height(0)
            , pendingRequestCount(0)
            , pendingConstructionCount(0)
            , activeSiteCount(0)
            , deliveryEventCount(0)
        {
        }
    };

} // namespace World
