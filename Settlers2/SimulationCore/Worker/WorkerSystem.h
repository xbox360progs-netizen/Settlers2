#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"

namespace World {

    struct WorldModel;

    class WorkerSystem : public ISimulationSystem {
    public:
        WorkerSystem();
        ~WorkerSystem();

        void Tick(WorldModel& world);

    private:
        uint32_t m_tickCount;
    };

}
