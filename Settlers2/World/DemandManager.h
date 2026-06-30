#pragma once
#include <list>
#include <stdint.h>
#include "Demand.h"

namespace World {
    class DemandManager {
    public:
        static const int MAX_TICKETS = 256;

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
        Demand* FindBestDemand(ResourceType type);

        void Clear();

    private:
        int AllocSlot();
        void FreeSlot(int index);

        std::list<Demand> m_demands;
        DemandTicket m_pool[MAX_TICKETS];
        int m_freeSlots[MAX_TICKETS];
        int m_freeCount;
        bool m_inUse[MAX_TICKETS];
        uint32_t m_nextTicketId;
    };
}
