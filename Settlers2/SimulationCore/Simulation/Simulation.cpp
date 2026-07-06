#include <stddef.h>
#include "Simulation.h"
#include "../Systems/ISimulationSystem.h"
#include "../Systems/EconomySystem.h"
#include "../Production/ProductionSystem.h"
#include "../Warehouse/WarehouseSystem.h"
#include "../Systems/DemandManager.h"
#include "../Systems/JobManager.h"
#include "../Worker/WorkerSystem.h"
#include "../Settlement/SettlementSystem.h"
#include "../Construction/ConstructionSystem.h"
#include "../Systems/ConsumptionSystem.h"
#include "../Systems/RenewableResourceSystem.h"
#include "../Transport/TransportController.h"
#include "../Transport/SimpleTransportDriver.h"
#include "../Transport/LocalTransferSystem.h"
#include "../World/WorldModel.h"
#include "../Construction/ConstructionState.h"
#include "../Stubs/DirectRouteRoadGraph.h"
#include "../Stubs/AcceptingFlagInventory.h"
#include "../Stubs/StubCargoRepository.h"
#include "../Stubs/StubDemandService.h"

namespace World {

    Simulation::Simulation(const SimulationConfig& config)
        : m_systemCount(0)
        , m_transport(NULL)
        , m_transportDriver(NULL)
        , m_demandManager(NULL)
        , m_world(NULL)
        , m_tickCount(config.initialTick)
        , m_roadGraph(NULL)
        , m_flagInventory(NULL)
        , m_cargoRepository(NULL)
        , m_demandService(NULL)
        , m_productionSystem(NULL)
        , m_economySystem(NULL)
        , m_warehouseSystem(NULL)
        , m_jobManager(NULL)
        , m_workerSystem(NULL)
        , m_settlementSystem(NULL)
        , m_constructionSystem(NULL)
        , m_consumptionSystem(NULL)
        , m_renewableSystem(NULL)
        , m_config(config)
    {
        // Create DemandManager first — may be needed as IDemandService by TransportController
        if (config.enableEconomy) {
            m_demandManager = new DemandManager();
            AddSystem(m_demandManager);
        }

        if (config.enableTransport) {
            m_roadGraph = new DirectRouteRoadGraph();
            m_flagInventory = new AcceptingFlagInventory();
            m_cargoRepository = new StubCargoRepository();

            // Use DemandManager as the demand service if available
            if (m_demandManager) {
                m_demandService = m_demandManager;
            } else {
                m_demandService = new StubDemandService();
            }

            m_transport = new TransportController(
                *m_roadGraph,
                *m_flagInventory,
                *m_cargoRepository,
                *m_demandService
            );

            m_transportDriver = new SimpleTransportDriver(*m_transport);
        }

        if (config.enableEconomy) {
            m_economySystem = new EconomySystem();
            AddSystem(m_economySystem);
        }

        if (config.enableTreeDepletion) {
            m_renewableSystem = new RenewableResourceSystem();
            m_renewableSystem->SetEnabled(true);
            AddSystem(m_renewableSystem);
        }

        if (config.enableConsumption) {
            m_consumptionSystem = new ConsumptionSystem();
            if (m_demandManager != NULL) {
                m_consumptionSystem->SetDemandManager(m_demandManager);
            }
            AddSystem(m_consumptionSystem);
        }

        if (config.enableProduction) {
            m_productionSystem = new ProductionSystem();
            m_productionSystem->SetRenewableSystem(m_renewableSystem);
            m_productionSystem->SetConsumptionEnabled(config.enableConsumption);
            if (m_demandManager != NULL) {
                m_productionSystem->SetDemandManager(m_demandManager);
            }
            AddSystem(m_productionSystem);
        }

        if (config.enableWorkers) {
            m_jobManager = new JobManager();
            AddSystem(m_jobManager);

            m_workerSystem = new WorkerSystem();
            m_workerSystem->SetJobManager(m_jobManager);
            AddSystem(m_workerSystem);
        }

        if (config.enableConstruction) {
            m_constructionSystem = new ConstructionSystem();
            if (m_demandManager != NULL) {
                m_constructionSystem->SetDemandManager(m_demandManager);
            }
            if (m_jobManager != NULL) {
                m_constructionSystem->SetJobManager(m_jobManager);
            }
            AddSystem(m_constructionSystem);
        }

        // LocalTransferSystem must tick before WarehouseSystem.
        // WarehouseSystem observes TransportNode state, never ProductionBuilding buffers.
        AddSystem(new LocalTransferSystem());

        if (config.enableWarehouse) {
            m_warehouseSystem = new WarehouseSystem();
            if (m_demandManager != NULL) {
                m_warehouseSystem->SetDemandManager(m_demandManager);
            }
            AddSystem(m_warehouseSystem);
        }

        if (config.enableSettlement) {
            m_settlementSystem = new SettlementSystem();
            if (m_jobManager != NULL) {
                m_settlementSystem->SetJobManager(m_jobManager);
            }
            if (m_economySystem != NULL) {
                m_settlementSystem->SetEconomySystem(m_economySystem);
            }
            AddSystem(m_settlementSystem);
        }
    }

    Simulation::~Simulation()
    {
        for (int i = 0; i < m_systemCount; ++i) {
            delete m_systems[i];
            m_systems[i] = NULL;
        }
        m_systemCount = 0;

        delete m_transportDriver;
        m_transportDriver = NULL;

        delete m_transport;
        m_transport = NULL;

        // m_demandService may point to m_demandManager; avoid double-delete
        // DemandManager is deleted via m_systems[] loop above (set to NULL below)
        if (m_demandService != m_demandManager) {
            delete m_demandService;
        }
        m_demandService = NULL;
        m_economySystem = NULL;
        m_workerSystem = NULL;
        m_settlementSystem = NULL;
        m_constructionSystem = NULL;
        m_consumptionSystem = NULL;
        m_renewableSystem = NULL;
        m_jobManager = NULL;
        m_demandManager = NULL;
        delete m_cargoRepository;
        m_cargoRepository = NULL;
        delete m_flagInventory;
        m_flagInventory = NULL;
        delete m_roadGraph;
        m_roadGraph = NULL;

        delete m_world;
        m_world = NULL;
    }

    void Simulation::AddSystem(ISimulationSystem* system)
    {
        if (m_systemCount >= kMaxSystems) return;
        m_systems[m_systemCount++] = system;
    }

    void Simulation::LoadWorld(const WorldModel& world)
    {
        delete m_world;
        m_world = new WorldModel(world);
        m_state.worldLoaded = true;
    }

    void Simulation::Tick()
    {
        if (!m_state.worldLoaded) return;

        ++m_tickCount;
        m_state.tickCount = m_tickCount;

        // 1. All domain systems — read WorldModel, write requests.
        //    Systems consume DeliveryEvent from the previous tick's transport.
        for (int i = 0; i < m_systemCount; ++i) {
            m_systems[i]->Tick(*m_world);
        }

        // 2. Clear old delivery and job events AFTER all consumers have processed them.
        ClearDeliveryEvents();
        ClearJobEvents();

        // 3. PR 3.8 — TransportController::Tick processes requests + drives carrier lifecycle
        if (m_transport) {
            m_transport->Tick(*m_world);
        }

        // 5. Capture economy telemetry from WorldModel
        m_state.economyPendingRequests = 0;
        m_state.economyFulfilledRequests = 0;
        for (int i = 0; i < m_world->pendingRequestCount; ++i) {
            if (m_world->pendingRequests[i].fulfilled)
                m_state.economyFulfilledRequests++;
            else
                m_state.economyPendingRequests++;
        }

        // 6. Capture construction telemetry from WorldModel
        m_state.constructionPendingRequests = 0;
        m_state.constructionActiveSites = 0;
        m_state.constructionCompletedSites = 0;
        for (int i = 0; i < m_world->pendingConstructionCount; ++i) {
            if (!m_world->pendingConstructionRequests[i].fulfilled)
                m_state.constructionPendingRequests++;
        }
        for (int i = 0; i < m_world->activeSiteCount; ++i) {
            if (m_world->activeSites[i].state == CS_Completed)
                m_state.constructionCompletedSites++;
            else
                m_state.constructionActiveSites++;
        }

        // 7. Capture new job events — published after WorkerSystem ticked in the system loop
        if (m_workerSystem) {
            m_workerSystem->CaptureJobEvents(*m_world);
        }

        // 8. Capture new delivery events for systems to consume on the next tick.
        if (m_transport) {
            CaptureDeliveryEvents();
            m_state.activeTransportTasks = m_transport->GetActiveTaskCount();
            m_state.blockedTransportTasks = m_transport->GetBlockedCount();
        }
    }

    void Simulation::ClearDeliveryEvents()
    {
        m_world->deliveryEventCount = 0;
    }

    void Simulation::ClearJobEvents()
    {
        m_world->jobEventCount = 0;
    }

    void Simulation::CaptureDeliveryEvents()
    {
        int count = m_transport->GetRecentDeliveryCount();
        for (int i = 0; i < count; ++i) {
            const TransportController::DeliveryRecord& rec = m_transport->GetRecentDelivery(i);
            if (m_world->deliveryEventCount >= kMaxDeliveryEvents) break;

            DeliveryEvent& ev = m_world->deliveryEvents[m_world->deliveryEventCount++];
            ev.type = DET_Completed;
            ev.resource = rec.resource;
            ev.amount = 1;
            ev.destinationFlag = rec.destinationFlag;
            ev.reason = rec.reason;
        }
        m_transport->ClearRecentDeliveries();
    }

    const SimulationState& Simulation::GetState() const
    {
        return m_state;
    }

    WorldModel& Simulation::GetWorld()
    {
        return *m_world;
    }

    int Simulation::GetCargoCount() const
    {
        if (m_transportDriver == NULL) return 0;
        return m_transportDriver->GetCargoCount();
    }

    const Cargo* Simulation::GetCargoAt(int index) const
    {
        if (m_transportDriver == NULL) return NULL;
        return m_transportDriver->GetCargoAt(index);
    }

} // namespace World
