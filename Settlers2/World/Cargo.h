#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "Handle.h"

namespace World {
    class Flag;
    struct DemandTicket;

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

        Cargo()
            : id(0), type(ResourceType_None), amount(0),
              state(Cargo_OnFlag), ticket(NULL) {}
    };
}
