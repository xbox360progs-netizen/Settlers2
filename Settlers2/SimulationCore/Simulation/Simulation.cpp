#include <stddef.h>
#include "Simulation.h"
#include "../Systems/EconomySystem.h"
#include "../Transport/TransportController.h"
#include "../World/WorldModel.h"
#include "../Stubs/StubRoadGraph.h"
#include "../Stubs/StubFlagInventory.h"
#include "../Stubs/StubCargoRepository.h"
#include "../Stubs/StubDemandService.h"

namespace World {

    Simulation::Simulation(const SimulationConfig& config)
        : m_transport(NULL)
        , m_economy(NULL)
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
            m_economy = new EconomySystem();
        }
    }

    Simulation::~Simulation()
    {
        delete m_economy;
        m_economy = NULL;

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

        // 1. Economy — generate demands
        if (m_economy) {
            m_economy->Tick(*m_world);
            m_state.economyPendingRequests = m_economy->GetState().pendingRequests;
            m_state.economyFulfilledRequests = m_economy->GetState().fulfilledRequests;
        }

        // 2. Convert pending requests to transport tasks
        ProcessTransportRequests();

        // 3. Transport — move cargo
        if (m_transport) {
            m_transport->Update(0.0f);
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

    const SimulationState& Simulation::GetState() const
    {
        return m_state;
    }

} // namespace World
