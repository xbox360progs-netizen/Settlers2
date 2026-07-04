#pragma once

namespace World {

    struct SimulationConfig;
    class Simulation;

    class ISimulationScenario {
    public:
        virtual ~ISimulationScenario() {}
        virtual const char* GetName() const = 0;
        virtual void Configure(SimulationConfig& config) const {}
        virtual void Initialize(Simulation& sim) = 0;
        virtual bool Tick(Simulation& sim) = 0;
    };

}
