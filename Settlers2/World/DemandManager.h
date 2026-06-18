#pragma once
#include <vector>
#include <stdint.h>
#include "Demand.h"

namespace World {
    class DemandManager {
    public:
        DemandManager();

        void SetDemand(ResourceType type, uint32_t amount, Handle<Flag> targetFlag, int priority);
        void ClearDemand(Handle<Flag> targetFlag);
        void ClearDemand(ResourceType type, Handle<Flag> targetFlag);

        DemandTicket* Reserve(ResourceType type);
        void ReleaseTicket(DemandTicket* ticket);
        void Deliver(DemandTicket* ticket);

        Demand* FindDemand(Handle<Flag> targetFlag);
        Demand* FindDemand(ResourceType type, Handle<Flag> targetFlag);
        bool HasDemand(ResourceType type);

        void Clear();

    private:
        std::vector<Demand> m_demands;
        std::vector<DemandTicket*> m_tickets;
        uint32_t m_nextTicketId;

        Demand* FindBestDemand(ResourceType type);
    };
}
