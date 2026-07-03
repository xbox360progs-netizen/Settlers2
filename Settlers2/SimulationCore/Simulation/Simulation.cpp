#include <stddef.h>
#include "Simulation.h"
#include "../Systems/ISimulationSystem.h"
#include "../Systems/EconomySystem.h"
#include "../Transport/TransportController.h"
#include "../World/WorldModel.h"
#include "../Construction/ConstructionState.h"
#include "../Stubs/StubRoadGraph.h"
#include "../Stubs/StubFlagInventory.h"
#include "../Stubs/StubCargoRepository.h"
#include "../Stubs/StubDemandService.h"

namespace World {

    Simulation::Simulation(const SimulationConfig& config)
        : m_systemCount(0)
        , m_transport(NULL)
        , m_world(NULL)
        , m_tickCount(config.initialTick)
        , m_stubRoadGraph(NULL)
        , m_stubFlagInventory(NULL)
        , m_stubCargoRepository(NULL)
        , m_stubDemandService(NULL)
    {
        if (config.enableTransport) {
            m_stubRoadGraph = new StubRoadGraph();
            m_stubFlagInventory = new StubFlagInventory();
            m_stubCargoRepository = new StubCargoRepository();
            m_stubDemandService = new StubDemandService();

            m_transport = new TransportController(
                *m_stubRoadGraph,
                *m_stubFlagInventory,
                *m_stubCargoRepository,
                *m_stubDemandService
            );
        }

        if (config.enableEconomy) {
            AddSystem(new EconomySystem());
        }
    }

    Simulation::~Simulation()
    {
        for (int i = 0; i < m_systemCount; ++i) {
            delete m_systems[i];
            m_systems[i] = NULL;
        }
        m_systemCount = 0;

        delete m_transport;
        m_transport = NULL;

        delete m_stubDemandService;
        m_stubDemandService = NULL;
        delete m_stubCargoRepository;
        m_stubCargoRepository = NULL;
        delete m_stubFlagInventory;
        m_stubFlagInventory = NULL;
        delete m_stubRoadGraph;
        m_stubRoadGraph = NULL;

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

        ClearDeliveryEvents();

        // 1. All domain systems — read WorldModel, write requests
        for (int i = 0; i < m_systemCount; ++i) {
            m_systems[i]->Tick(*m_world);
        }

        // 2. Convert pending requests to transport tasks
        ProcessTransportRequests();

        // 3. Capture economy telemetry from WorldModel
        m_state.economyPendingRequests = 0;
        m_state.economyFulfilledRequests = 0;
        for (int i = 0; i < m_world->pendingRequestCount; ++i) {
            if (m_world->pendingRequests[i].fulfilled)
                m_state.economyFulfilledRequests++;
            else
                m_state.economyPendingRequests++;
        }

        // 4. Capture construction telemetry from WorldModel
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

        // 5. Transport — move cargo
        if (m_transport) {
            m_transport->Update(0.0f);
            CaptureDeliveryEvents();
            m_state.activeTransportTasks = m_transport->GetActiveTaskCount();
            m_state.blockedTransportTasks = m_transport->GetBlockedCount();
        }
    }

    void Simulation::ProcessTransportRequests()
    {
        if (!m_world || !m_transport) return;

        for (int i = 0; i < m_world->pendingRequestCount; ++i) {
            TransportRequest& req = m_world->pendingRequests[i];
            if (req.fulfilled) continue;

            TransportTask* task = m_transport->CreateTask(
                req.resource, req.origin, req.destination, req.reason);

            if (task != NULL) {
                req.fulfilled = true;
            }
        }
    }

    void Simulation::ClearDeliveryEvents()
    {
        m_world->deliveryEventCount = 0;
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

} // namespace World
