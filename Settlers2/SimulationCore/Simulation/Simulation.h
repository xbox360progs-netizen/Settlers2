#pragma once
#include "SimulationConfig.h"
#include "SimulationState.h"

namespace World {

    class TransportController;
    class ISimulationSystem;
    struct WorldModel;
    class IRoadGraph;
    class IFlagInventory;
    class ICargoRepository;
    class IDemandService;

    class Simulation {
    public:
        Simulation(const SimulationConfig& config);
        ~Simulation();

        void AddSystem(ISimulationSystem* system);
        void LoadWorld(const WorldModel& world);
        void Tick();
        const SimulationState& GetState() const;
        WorldModel& GetWorld();

    private:
        void ProcessTransportRequests();
        void ClearDeliveryEvents();
        void CaptureDeliveryEvents();

        static const int kMaxSystems = 16;
        ISimulationSystem* m_systems[kMaxSystems];
        int m_systemCount;

        TransportController* m_transport;
        WorldModel* m_world;
        SimulationState m_state;
        uint32_t m_tickCount;

        IRoadGraph* m_stubRoadGraph;
        IFlagInventory* m_stubFlagInventory;
        ICargoRepository* m_stubCargoRepository;
        IDemandService* m_stubDemandService;
    };

} // namespace World
