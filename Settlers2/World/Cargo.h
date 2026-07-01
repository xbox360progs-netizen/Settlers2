#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "Handle.h"

namespace World {
    class Flag;
    struct DemandTicket;
    struct TransportTask;

    enum CargoState {
        Cargo_OnFlag,
        Cargo_Carried,
        Cargo_Delivered
    };

    struct Cargo {
        uint32_t id;
        ResourceType type;
        uint32_t amount;
        CargoState state;
        Handle<Flag> currentFlag;
        DemandTicket* ticket;

        // Phase 7 — ownership link back to the task that owns this cargo.
        // Set by Controller::NotifyCarrierPickedUp, cleared on delivery.
        TransportTask* ownerTask;

        Cargo()
            : id(0), type(ResourceType_None), amount(0),
              state(Cargo_OnFlag), ticket(NULL), ownerTask(NULL) {}
    };
}
