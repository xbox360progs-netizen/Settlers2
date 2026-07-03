#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"

namespace World {

    struct WorldModel;
    struct ConstructionSite;

    class ConstructionSystem : public ISimulationSystem {
    public:
        ConstructionSystem();
        ~ConstructionSystem();

        void Tick(WorldModel& world);

    private:
        void GenerateRequests(WorldModel& world);
        void ProcessRequests(WorldModel& world);
        void UpdateSites(WorldModel& world);
        void ProcessDeliveryEvents(WorldModel& world);
        void InitializeSiteResources(ConstructionSite& site);
        void PublishResourceRequests(WorldModel& world);

        uint32_t m_tickCount;
    };

}
