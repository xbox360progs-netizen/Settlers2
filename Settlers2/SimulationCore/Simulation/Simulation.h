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
    class ProductionSystem;
    class EconomySystem;
    class WarehouseSystem;
    class SimpleTransportDriver;
    class DemandManager;
    class JobManager;
    class WorkerSystem;
    class SettlementSystem;
    class ConstructionSystem;

    class ConsumptionSystem;
    class RenewableResourceSystem;

    class Simulation {
    public:
        Simulation(const SimulationConfig& config);
        ~Simulation();

        void AddSystem(ISimulationSystem* system);
        void LoadWorld(const WorldModel& world);
        void Tick();
        const SimulationState& GetState() const;
        WorldModel& GetWorld();
        DemandManager* GetDemandManager() { return m_demandManager; }
        EconomySystem* GetEconomySystem() { return m_economySystem; }
        WarehouseSystem* GetWarehouseSystem() { return m_warehouseSystem; }
        JobManager* GetJobManager() { return m_jobManager; }
        WorkerSystem* GetWorkerSystem() { return m_workerSystem; }
        SettlementSystem* GetSettlementSystem() { return m_settlementSystem; }
        ConstructionSystem* GetConstructionSystem() { return m_constructionSystem; }
        ConsumptionSystem* GetConsumptionSystem() { return m_consumptionSystem; }
        RenewableResourceSystem* GetRenewableResourceSystem() { return m_renewableSystem; }
        const SimulationConfig& GetConfig() const { return m_config; }

    private:
        void ProcessTransportRequests();
        void ClearDeliveryEvents();
        void ClearJobEvents();
        void CaptureDeliveryEvents();

        static const int kMaxSystems = 16;
        ISimulationSystem* m_systems[kMaxSystems];
        int m_systemCount;

        TransportController* m_transport;
        SimpleTransportDriver* m_transportDriver;
        DemandManager* m_demandManager;
        WorldModel* m_world;
        SimulationState m_state;
        uint32_t m_tickCount;

        IRoadGraph* m_roadGraph;
        IFlagInventory* m_flagInventory;
        ICargoRepository* m_cargoRepository;
        IDemandService* m_demandService;

        ProductionSystem* m_productionSystem;
        EconomySystem* m_economySystem;
        WarehouseSystem* m_warehouseSystem;
        JobManager* m_jobManager;
        WorkerSystem* m_workerSystem;
        SettlementSystem* m_settlementSystem;
        ConstructionSystem* m_constructionSystem;
        ConsumptionSystem* m_consumptionSystem;
        RenewableResourceSystem* m_renewableSystem;
        SimulationConfig m_config;
    };

} // namespace World
