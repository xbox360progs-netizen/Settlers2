#include "stdafx.h"
#include "SimulationSystem.h"
#include "../ConstructionManager.h"
#include "../CarrierManager.h"
#include "../CarrierSystem.h"
#include "../WorkerManager.h"
#include "../TransportJobManager.h"
#include "../CargoManager.h"
#include "../DemandManager.h"
#include "../StorehouseManager.h"
#include "../Flag.h"
#include "../EntityManager.h"
#include "../Map.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../../Logic/EconomyManager.h"
#include "../../Core/EventBus.h"

namespace World {

SimulationSystem::SimulationSystem()
    : m_eventBus(NULL)
    , m_initialized(false)
    , m_externalMode(false)
    , m_extConstruction(NULL)
    , m_extEconomy(NULL)
    , m_extCarriers(NULL)
    , m_extCarrierSystem(NULL)
    , m_extWorkers(NULL)
    , m_extTransportJobs(NULL)
    , m_extCargo(NULL)
    , m_extDemand(NULL)
    , m_extStorehouse(NULL)
{
}

SimulationSystem::~SimulationSystem()
{
}

void SimulationSystem::SetExternalManagers(
    ConstructionManager* construction,
    Logic::EconomyManager* economy,
    CarrierManager* carriers,
    CarrierSystem* carrierSystem,
    WorkerManager* workers,
    TransportJobManager* transportJobs,
    CargoManager* cargo,
    DemandManager* demand,
    StorehouseManager* storehouse)
{
    m_extConstruction = construction;
    m_extEconomy = economy;
    m_extCarriers = carriers;
    m_extCarrierSystem = carrierSystem;
    m_extWorkers = workers;
    m_extTransportJobs = transportJobs;
    m_extCargo = cargo;
    m_extDemand = demand;
    m_extStorehouse = storehouse;
    m_externalMode = true;
}

void SimulationSystem::Initialize(
    Map* map,
    EntityManager* entityManager,
    FlagManager* flagManager,
    RoadManager* roadManager,
    Flag* warehouseFlag,
    Warehouse* warehouse,
    Core::EventBus* eventBus)
{
    m_eventBus = eventBus;

    if (!m_eventBus) {
        m_eventBus = new Core::EventBus();
    }

    if (m_externalMode) {
        // Wire external EconomyManager into the EconomySystem wrapper
        m_economy.SetManager(m_extEconomy);
        m_economy.Initialize(m_eventBus);

        // Wire external managers into TransportSystem
        if (m_extCarriers && m_extCarrierSystem) {
            m_transport.SetExternalManagers(
                m_extCarriers, m_extCarrierSystem, m_extTransportJobs,
                m_extCargo, m_extDemand, flagManager, roadManager);
        }
        m_transport.Initialize(entityManager, flagManager, roadManager, m_eventBus);

        // Wire external WorkerManager into WorkforceSystem
        if (m_extWorkers) {
            m_workforce.SetExternalManager(m_extWorkers);
        }
        m_workforce.Initialize(roadManager, map, m_eventBus);

        m_buildings.Initialize(map, flagManager, m_eventBus);

        m_construction.Initialize(
            flagManager, roadManager,
            m_extDemand, m_extCargo,
            warehouseFlag, m_eventBus);

        // Initialize WorldSystem
        m_world.Initialize(map, flagManager, m_extCargo, m_eventBus);

        // Wire up cross-system references
        if (m_extCargo) {
            m_economy.SetCargoManager(m_extCargo);
        }
        if (m_extDemand) {
            m_economy.SetDemandManager(m_extDemand);
        }
    }

    m_initialized = true;
}

void SimulationSystem::Update(float dt)
{
    if (!m_initialized) return;

    if (m_externalMode) {
        // Phase 1: Construction — resource transfer, builder movement, progress
        if (m_extConstruction) {
            m_extConstruction->Update(dt);
        }

        // Phase 2: Economy — requests, production, distribution
        if (m_extConstruction && m_extEconomy) {
            m_extConstruction->GenerateRequests(m_extEconomy);
        }
        if (m_extEconomy) {
            m_extEconomy->Update(dt);
        }

        // Phase 3: Workforce — worker arrivals
        if (m_extWorkers) {
            m_extWorkers->Update(dt);
        }

        // Phase 4: Transport — carrier walking (CarrierSystem is synced internally by CarrierManager)
        if (m_extCarriers) {
            m_extCarriers->Update(dt);
        }

        // Phase 5: Warehouse collection
        if (m_extEconomy) {
            m_extEconomy->CollectWarehouse();
        }

        // Phase 6: Construction post-update — collect completed sites
        m_construction.PostUpdate();

        // Phase 7: Event dispatch — deliver queued events to all listeners
        // (Flush runs after PostUpdate so completion events reach listeners
        // before World phase mutates the map)
        if (m_eventBus) {
            m_eventBus->Flush();
        }

        // Phase 8: World — tree growth, wildlife regeneration
        m_world.Update(dt);
    } else {
        // Owned mode (future: SimulationSystem owns managers directly)
        m_construction.Update(dt);
        m_construction.GenerateRequests(m_economy.GetManager());
        m_economy.Update(dt);
        m_workforce.Update(dt);
        m_transport.Update(dt);
        m_economy.CollectWarehouse();
    }

}

} // namespace World
