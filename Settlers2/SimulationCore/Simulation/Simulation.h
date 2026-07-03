#pragma once
#include "SimulationConfig.h"
#include "SimulationState.h"

namespace World {

    class TransportController;
    class EconomySystem;
    struct WorldModel;
    class IRoadGraph;
    class IFlagInventory;
    class ICargoRepository;
    class IDemandService;

    class Simulation {
    public:
        Simulation(const SimulationConfig& config);
        ~Simulation();

        void LoadWorld(const WorldModel& world);
        void Tick();
        const SimulationState& GetState() const;

    private:
        void ProcessTransportRequests();

        TransportController* m_transport;
        EconomySystem* m_economy;
        WorldModel* m_world;
        SimulationState m_state;
        uint32_t m_tickCount;

        IRoadGraph* m_stubRoadGraph;
        IFlagInventory* m_stubFlagInventory;
        ICargoRepository* m_stubCargoRepository;
        IDemandService* m_stubDemandService;
    };

} // namespace World
