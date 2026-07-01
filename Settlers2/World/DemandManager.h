#pragma once
#include <list>
#include <stdint.h>
#include "Demand.h"
#include "TransportTypes.h"

namespace World {
    class TransportController;
    class FlagManager;

    class DemandManager {
    public:
        static const int MAX_TICKETS = 256;

        DemandManager();

        void SetTransportController(TransportController* tc) { m_controller = tc; }
        TransportController* GetController() const { return m_controller; }

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }

        void SetDemand(ResourceType type, uint32_t amount, Handle<Flag> targetFlag, int priority);
        void ClearDemand(Handle<Flag> targetFlag);
        void ClearDemand(ResourceType type, Handle<Flag> targetFlag);

        DemandTicket* Reserve(ResourceType type, FlagId originFlag = 0);
        void ReleaseTicket(DemandTicket* ticket);
        void Deliver(DemandTicket* ticket);

        // Phase 8.2 — resolve ticket by ID (used for delivery accounting via task→ticket link)
        DemandTicket* GetTicket(uint32_t ticketId) {
            for (int i = 0; i < MAX_TICKETS; ++i)
                if (m_inUse[i] && m_pool[i].id == ticketId)
                    return &m_pool[i];
            return NULL;
        }

        Demand* FindDemand(Handle<Flag> targetFlag);
        Demand* FindDemand(ResourceType type, Handle<Flag> targetFlag);
        bool HasDemand(ResourceType type);
        bool HasDemandFromOtherFlag(ResourceType type, Handle<Flag> currentFlag);
        Demand* FindBestDemand(ResourceType type);

        void Clear();

    private:
        int AllocSlot();
        void FreeSlot(int index);

        TransportTaskReason ReasonForResource(ResourceType type) const;

        std::list<Demand> m_demands;
        DemandTicket m_pool[MAX_TICKETS];
        int m_freeSlots[MAX_TICKETS];
        int m_freeCount;
        bool m_inUse[MAX_TICKETS];
        uint32_t m_nextTicketId;
        TransportController* m_controller;   // Phase 8.1 — bridge
        FlagManager* m_flagManager;          // Resolve handle→FlagId for CreateTask
    };
}
