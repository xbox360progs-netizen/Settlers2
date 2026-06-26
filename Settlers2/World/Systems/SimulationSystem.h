#pragma once
#include "ConstructionSystem.h"
#include "EconomySystem.h"
#include "TransportSystem.h"
#include "WorkforceSystem.h"
#include "BuildingSystem.h"
#include "WorldSystem.h"
#include "../../Core/EventBus.h"
#include "../../Core/JobManager.h" // JobManager is in global namespace

namespace Logic {
    class EconomyManager;
    class AISystem;
}

namespace World {
    class Map;
    class FlagManager;
    class RoadManager;
    class EntityManager;
    class ConstructionManager;
    class CarrierManager;
    class CarrierSystem;
    class WorkerManager;
    class CargoManager;
    class DemandManager;
    class TransportJobManager;
    class StorehouseManager;
    class Flag;
    class Warehouse;
}

namespace World {

class SimulationSystem {
public:
    SimulationSystem();
    ~SimulationSystem();

    // External pointer mode: accept existing managers from GameScene
    // This allows gradual migration without rewriting all creation code at once.
    void SetExternalManagers(
        ConstructionManager* construction,
        Logic::EconomyManager* economy,
        CarrierManager* carriers,
        CarrierSystem* carrierSystem,
        WorkerManager* workers,
        TransportJobManager* transportJobs,
        CargoManager* cargo,
        DemandManager* demand,
        StorehouseManager* storehouse);

    void Initialize(
        Map* map,
        EntityManager* entityManager,
        FlagManager* flagManager,
        RoadManager* roadManager,
        Flag* warehouseFlag,
        Warehouse* warehouse,
        Core::EventBus* eventBus);

    void Update(float dt);

    // Individual system accessors
    ConstructionSystem& GetConstruction() { return m_construction; }
    EconomySystem& GetEconomy() { return m_economy; }
    TransportSystem& GetTransport() { return m_transport; }
    WorkforceSystem& GetWorkforce() { return m_workforce; }
    BuildingSystem& GetBuildings() { return m_buildings; }
    WorldSystem& GetWorld() { return m_world; }

    const ConstructionSystem& GetConstruction() const { return m_construction; }
    const EconomySystem& GetEconomy() const { return m_economy; }
    const TransportSystem& GetTransport() const { return m_transport; }
    const WorkforceSystem& GetWorkforce() const { return m_workforce; }
    const BuildingSystem& GetBuildings() const { return m_buildings; }

    Core::EventBus* GetEventBus() { return m_eventBus; }

    void SetJobManager(JobManager* jm) { m_jobManager = jm; }
    void SetAISystem(Logic::AISystem* ai) { m_extAi = ai; }

    bool IsInitialized() const { return m_initialized; }

private:
    ConstructionSystem m_construction;
    EconomySystem m_economy;
    TransportSystem m_transport;
    WorkforceSystem m_workforce;
    BuildingSystem m_buildings;
    WorldSystem m_world;

    Core::EventBus* m_eventBus;
    bool m_initialized;
    bool m_externalMode;

    // External pointers (non-owning, set via SetExternalManagers)
    ConstructionManager* m_extConstruction;
    Logic::EconomyManager* m_extEconomy;
    CarrierManager* m_extCarriers;
    CarrierSystem* m_extCarrierSystem;
    WorkerManager* m_extWorkers;
    TransportJobManager* m_extTransportJobs;
    CargoManager* m_extCargo;
    DemandManager* m_extDemand;
    StorehouseManager* m_extStorehouse;

    // Job system (optional, used for parallel AI planning)
    JobManager* m_jobManager;
    Logic::AISystem* m_extAi;
};

/// Maximum number of build requests per AI chunk
static const int MAX_AI_REQUESTS_PER_CHUNK = 8;

} // namespace World
