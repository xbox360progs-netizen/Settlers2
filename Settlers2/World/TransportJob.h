#pragma once
#include "ResourceNode.h"
#include <vector>
#include <stdint.h>
#include "Flag.h"
#include "Handle.h"

namespace World {
    class Carrier;

    struct Cargo {
        ResourceType type;
        uint8_t amount;
        uint32_t destFlagId; // 0 = no destination (free resource)

        Cargo() : type(ResourceType_None), amount(0), destFlagId(0) {}
        Cargo(ResourceType t, uint8_t a, uint32_t d = 0) : type(t), amount(a), destFlagId(d) {}
    };

    struct TransportJob {
        uint32_t id;

        ResourceType resource;
        uint32_t amount;
        uint32_t cargoId;

        FlagHandle sourceFlag;
        FlagHandle destinationFlag;

        std::vector<FlagHandle> route;
        uint32_t currentLeg;

        Handle<Carrier> assignedCarrier;

        enum State {
            Waiting,
            Assigned,
            InTransit,
            Delivered,
            Cancelled
        };

        State state;

        TransportJob()
            : id(0), resource(ResourceType_None), amount(0), cargoId(0),
              currentLeg(0), state(Waiting) {}
    };
}
