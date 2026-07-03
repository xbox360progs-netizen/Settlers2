#include "WorkerSystem.h"
#include "../World/WorldModel.h"

namespace World {

    WorkerSystem::WorkerSystem()
        : m_tickCount(0)
    {
    }

    WorkerSystem::~WorkerSystem()
    {
    }

    void WorkerSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        for (int i = 0; i < world.activeSiteCount; ++i) {
            ConstructionSite& site = world.activeSites[i];
            if (site.state != CS_Building) continue;
            if (site.builderAssigned) continue;

            site.builderAssigned = true;
        }
    }

} // namespace World
