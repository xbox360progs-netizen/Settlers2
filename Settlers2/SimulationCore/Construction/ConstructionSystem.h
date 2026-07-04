#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"

namespace World {

    struct WorldModel;
    struct ConstructionSite;
    class DemandManager;
    class JobManager;

    class ConstructionSystem : public ISimulationSystem {
    public:
        ConstructionSystem();
        ~ConstructionSystem();

        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }
        void SetJobManager(JobManager* jm) { m_jobManager = jm; }

        void Tick(WorldModel& world);

    private:
        void ProcessJobEvents(WorldModel& world);
        void GenerateRequests(WorldModel& world);
        void ProcessRequests(WorldModel& world);
        void UpdateSites(WorldModel& world, uint32_t currentTick);
        void CompleteSites(WorldModel& world);
        void ProcessDeliveryEvents(WorldModel& world);
        void InitializeSiteResources(ConstructionSite& site);
        void RequestResources(WorldModel& world);

        uint32_t m_tickCount;
        DemandManager* m_demandManager;
        JobManager* m_jobManager;
    };

}
