#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "../Core/Handle.h"

namespace World {
    class Flag;
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

        // Phase 8.2 — ownership link to the TransportTask that owns this cargo.
        TransportTask* ownerTask;

        Cargo()
            : id(0), type(ResourceType_None), amount(0),
              state(Cargo_OnFlag), ownerTask(NULL) {}
    };
}
