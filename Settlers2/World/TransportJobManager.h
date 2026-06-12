#pragma once
#include "TransportJob.h"
#include <vector>
#include <stdint.h>

//#define ROUTE_RECALC_DEBUG

namespace World {
    class Flag;
    class FlagManager;
    class RoadManager;
    class CarrierManager;
    struct Road;
    class Carrier;

    struct InTransitCounts {
        uint32_t count[ResourceType_Count];
        InTransitCounts() { for (int i = 0; i < ResourceType_Count; ++i) count[i] = 0; }
    };

    class TransportJobManager {
    public:
        TransportJobManager();
        ~TransportJobManager();

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetCarrierManager(CarrierManager* cm) { m_carrierManager = cm; }

        TransportJob* CreateJob(ResourceType resource, uint32_t amount,
                                Flag* source, Flag* destination);
        void CancelJob(TransportJob* job);
        void CancelJobsForFlag(Flag* flag);

        void Update();

        void OnLegDelivered(TransportJob* job);

        TransportJob* GetJob(uint32_t id) const;
        size_t GetJobCount() const { return m_jobs.size(); }
        TransportJob* GetJobByIndex(size_t i) const {
            return (i < m_jobs.size()) ? m_jobs[i] : NULL;
        }

        // Bridge: scan flags for cargo with destFlagId and create jobs
        void ScanFlagsForCargo(FlagManager* flagManager);

        // Deferred route recalculation
        void RecalculateRoutes();
        void MarkRoutesDirty() { m_routesDirty = true; }
        bool IsRoutesDirty() const { return m_routesDirty; }
        void FlushRecalculate();
        bool IsRoadInUse(Road* road) const;
        bool IsFlagInUse(Flag* flag) const;

    private:
        FlagManager* m_flagManager;
        RoadManager* m_roadManager;
        CarrierManager* m_carrierManager;

        std::vector<TransportJob*> m_jobs;
        uint32_t m_nextJobId;

        std::vector<std::vector<InTransitCounts>> m_inTransitCount;
        bool m_routesDirty;
        bool m_recalculatingRoutes;

        void EnsureInTransitSize(uint32_t srcFlagId, uint32_t destFlagId);
        TransportJob* FindJobForRoad(Road* road) const;
    };
}
