#pragma once

namespace World {

    struct WorldModel;

    class ISimulationSystem {
    public:
        virtual ~ISimulationSystem() {}
        virtual void Tick(WorldModel& world) = 0;
    };

} // namespace World
