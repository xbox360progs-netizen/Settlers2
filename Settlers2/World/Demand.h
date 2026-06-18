#pragma once
#include <stdint.h>
#include <vector>
#include "ResourceNode.h"
#include "Handle.h"

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

        DemandTicket() : id(0), type(ResourceType_None), demand(NULL), state(Ticket_Active) {}
    };
}

#if 0
// Example usage:
// DemandManager dm;
// dm.SetDemand(ResourceType_Wood, 3, constructionFlag->GetHandle(), 100);
// DemandTicket* ticket = dm.Reserve(ResourceType_Wood);
// if (ticket) { cargo->ticket = ticket; }
// ...
// dm.Deliver(ticket);  // when cargo arrives at target
// dm.ReleaseTicket(ticket);  // if cargo is lost
#endif
