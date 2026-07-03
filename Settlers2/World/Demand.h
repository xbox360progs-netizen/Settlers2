#pragma once
#include <stdint.h>
#include <vector>
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Core/Handle.h"

namespace World {
    class Flag;

    struct Demand {
        ResourceType type;
        uint32_t requested;
        uint32_t reserved;
        uint32_t delivered;
        Handle<Flag> targetFlag;
        int priority;
    };

    enum TicketState {
        Ticket_Active,
        Ticket_Cancelled,
        Ticket_Delivered
    };

    struct DemandTicket {
        uint32_t id;
        ResourceType type;
        Demand* demand;
        TicketState state;
        uint32_t transportTaskId;       // Phase 8.1 — bridge to TransportController (0 = no task)

        DemandTicket() : id(0), type(ResourceType_None), demand(NULL), state(Ticket_Active), transportTaskId(0) {}
    };
}

#if 0
// Example usage (Phase 8.2 — DemandTicket is internal to DemandManager):
// DemandManager dm;
// dm.SetDemand(ResourceType_Wood, 3, constructionFlag->GetHandle(), 100);
// DemandTicket* ticket = dm.Reserve(ResourceType_Wood);
//   → creates TransportTask, ticket stays in DemandManager pool
// ...
// dm.Deliver(ticket);  // when cargo arrives at target
// dm.ReleaseTicket(ticket);  // if cargo is lost
#endif
