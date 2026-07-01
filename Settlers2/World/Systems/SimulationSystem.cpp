#include "stdafx.h"
#include "SimulationSystem.h"
#include "../ConstructionManager.h"
#include "../CarrierManager.h"
#include "CarrierSystem.h"
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
#include "../../Logic/AISystem.h"
#include "../../Core/EventBus.h"
#include "../../Core/JobManager.h"

namespace World {

struct AIChunkData
{
    Logic::AISystem* ai;
    BuildingType types[4];
    int numTypes;
    Logic::BuildRequest requests[MAX_AI_REQUESTS_PER_CHUNK];
    int numRequests;
};

static void AIChunkJobFunc(void* data)
{
    AIChunkData* d = static_cast<AIChunkData*>(data);
    for (int i = 0; i < d->numTypes; ++i)
    {
        if (d->numRequests >= MAX_AI_REQUESTS_PER_CHUNK) break;
        Logic::BuildRequest req;
        if (d->ai->PlanBuild(d->types[i], req))
            d->requests[d->numRequests++] = req;
    }
}

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
        , m_extTransportCtrl(NULL)
        , m_extCargo(NULL)
    , m_extDemand(NULL)
    , m_extStorehouse(NULL)
    , m_jobManager(NULL)
    , m_extAi(NULL)
{
}

SimulationSystem::~SimulationSystem()
{
    if (m_jobManager) {
        m_jobManager->Shutdown();
        delete m_jobManager;
        m_jobManager = NULL;
    }
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
    StorehouseManager* storehouse,
    TransportController* transportCtrl)
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
    m_extTransportCtrl = transportCtrl;
    m_externalMode = true;
}

void SimulationSystem::Initialize(
    Map* map,
    EntityManager* entityManager,
    FlagManager* flagManager,
    RoadManager* roadManager,
    Flag* warehouseFlag,
    Warehouse* warehouse,
    Core::EventBus* eventBus,
    Core::CommandBus* commandBus)
{
    m_eventBus = eventBus;
    m_commandBus = commandBus;

    if (!m_eventBus) {
        m_eventBus = new Core::EventBus();
    }
    if (!m_commandBus) {
        m_commandBus = new Core::CommandBus();
    }
    m_commandBus->SetEventBus(m_eventBus);

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

        m_buildings.Initialize(map, flagManager, m_eventBus, m_commandBus);
        if (m_extEconomy) m_buildings.SetEconomyManager(m_extEconomy);
        if (m_extWorkers) m_buildings.SetWorkerManager(m_extWorkers);

        {
            BuildContext ctx(flagManager, roadManager, m_extDemand, m_extCargo, m_extCarriers, map, warehouseFlag);
            m_construction.Initialize(ctx, m_eventBus, m_commandBus);

            // CRITICAL: Redirect external pointer to the internal ConstructionManager
            // so Phase 1/2 (m_extConstruction->Update/GenerateRequests) read from the
            // same instance where the command handler adds sites.
            // Without this, HandlePlaceFlag→Enqueue→AddSite adds to the internal
            // m_construction.m_manager, but Phase 1/2 read from the empty external cm.
            m_extConstruction = m_construction.GetManager();
        }

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

        // Phase 4B: TransportController — lifecycle, dispatch, telemetry
        if (m_extTransportCtrl) {
            m_extTransportCtrl->Update(dt);
        }

        // Phase 5: Warehouse collection
        if (m_extEconomy) {
            m_extEconomy->CollectWarehouse();
        }

        // Phase 5B: AI planning (parallel via JobManager when available)
        if (m_extAi && m_jobManager) {
            m_extAi->ClearReservations();

            AIChunkData chunks[4];
            for (int c = 0; c < 4; ++c) {
                chunks[c].ai = m_extAi;
                chunks[c].numRequests = 0;
            }
            chunks[0].types[0] = Woodcutter;
            chunks[0].types[1] = Sawmill;
            chunks[0].types[2] = CoalMine;
            chunks[0].numTypes = 3;

            chunks[1].types[0] = IronMine;
            chunks[1].types[1] = IronSmelter;
            chunks[1].types[2] = ToolWorkshop;
            chunks[1].numTypes = 3;

            chunks[2].types[0] = Farm;
            chunks[2].types[1] = Mill;
            chunks[2].types[2] = Bakery;
            chunks[2].numTypes = 3;

            chunks[3].types[0] = Hunter;
            chunks[3].types[1] = Fisher;
            chunks[3].types[2] = GoldMine;
            chunks[3].types[3] = GoldSmelter;
            chunks[3].numTypes = 4;

            for (int c = 0; c < 4; ++c)
                m_jobManager->Submit(AIChunkJobFunc, &chunks[c]);
            m_jobManager->WaitAll();

            for (int c = 0; c < 4; ++c)
                m_extAi->ApplyBuildRequests(chunks[c].requests, chunks[c].numRequests);
        }

        // Phase 6: Command dispatch — drain all pending commands first.
        // Commands represent intent (PlaceFlag, DeleteFlag) and mutate world state.
        // They must be consumed before events are dispatched.
        if (m_commandBus) {
            while (m_commandBus->Flush()) { }
        }

        // Phase 7: Construction post-update — collect completed sites
        m_construction.PostUpdate();

        // Phase 8: Event dispatch — drain all pending events
        // Loop ensures cascading events from listeners are delivered within
        // the same frame.  Flush returns false when the queue is empty.
        // Depth guard inside Flush() prevents runaway recursion when a
        // listener calls Flush() directly.
        if (m_eventBus) {
            while (m_eventBus->Flush()) { }
        }

        // Phase 9: World — tree growth, wildlife regeneration
        m_world.Update(dt);
    } else {
        // Owned mode (future: SimulationSystem owns managers directly)
        m_construction.Update(dt);
        m_construction.GenerateRequests(m_economy.GetManager());
        m_economy.Update(dt);
        m_workforce.Update(dt);
        m_transport.Update(dt);
        m_economy.CollectWarehouse();

        // Command dispatch
        if (m_commandBus) {
            while (m_commandBus->Flush()) { }
        }

        // Event dispatch
        if (m_eventBus) {
            while (m_eventBus->Flush()) { }
        }
    }

}

} // namespace World
