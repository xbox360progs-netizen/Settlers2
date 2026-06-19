#pragma once
#include <stdint.h>
#include "TransportJob.h"

#define MAX_TRANSPORT_JOBS 512

namespace World {
    class FlagManager;
    class RoadManager;
    class CarrierManager;
    struct Road;
    class Warehouse;

    class TransportJobManager {
    public:
        TransportJobManager();
        ~TransportJobManager();

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetCarrierManager(CarrierManager* cm) { m_carrierManager = cm; }
        void SetWarehouse(Warehouse* wh) { m_warehouse = wh; }

        // Pool-based job management
        TransportJob* CreateJob(ResourceType rType, Flag* src, Flag* dst);
        void FreeJob(TransportJob* job);

        int GetActiveCount() const { return m_activeCount; }
        TransportJob* GetJobByActiveIdx(int i) const { return const_cast<TransportJob*>(&m_pool[m_activeIndices[i]]); }

        void Clear();

        // Deletion safety stubs (real logic lives in CarrierManager)
        bool IsRoadInUse(Road* road) const { return false; }
        bool IsFlagInUse(Flag* flag) const { return false; }

    private:
        TransportJob m_pool[MAX_TRANSPORT_JOBS];
        uint32_t m_activeIndices[MAX_TRANSPORT_JOBS];
        int m_activeCount;
        uint32_t m_freeSlots[MAX_TRANSPORT_JOBS];
        int m_freeCount;
        uint32_t m_nextJobId;

        FlagManager* m_flagManager;
        RoadManager* m_roadManager;
        CarrierManager* m_carrierManager;
        Warehouse* m_warehouse;
    };
}
