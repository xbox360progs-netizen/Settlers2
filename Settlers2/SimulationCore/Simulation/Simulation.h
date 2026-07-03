#pragma once
#include "SimulationConfig.h"
#include "SimulationState.h"

namespace World {

    class TransportController;
    class WorldModel;
    class IRoadGraph;
    class IFlagInventory;
    class ICargoRepository;
    class IDemandService;

    // Single entry point for simulation.
    // Composes all simulation systems and coordinates their tick order.
    // Has no dependency on rendering, input, audio, or any graphics API.
    class Simulation {
    public:
        Simulation(const SimulationConfig& config);
        ~Simulation();

        void LoadWorld(const WorldModel& world);
        void Tick();
        const SimulationState& GetState() const;

    private:
        TransportController* m_transport;
        WorldModel* m_world;
        SimulationState m_state;
        uint32_t m_tickCount;

        // Stub interface implementations (for headless simulation).
        // Replaced by real adapters when SimulationCore runs inside the game.
        IRoadGraph* m_stubRoadGraph;
        IFlagInventory* m_stubFlagInventory;
        ICargoRepository* m_stubCargoRepository;
        IDemandService* m_stubDemandService;

        // Future systems:
        // EconomySystem* m_economy;
        // ConstructionSystem* m_construction;
        // WorkerSystem* m_workers;
    };

} // namespace World
