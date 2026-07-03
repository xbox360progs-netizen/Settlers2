#include "Simulation.h"
#include "../Transport/TransportController.h"
#include "../World/WorldModel.h"
#include "../Stubs/StubRoadGraph.h"
#include "../Stubs/StubFlagInventory.h"
#include "../Stubs/StubCargoRepository.h"
#include "../Stubs/StubDemandService.h"

namespace World {

    Simulation::Simulation(const SimulationConfig& config)
        : m_transport(NULL)
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
    }

    Simulation::~Simulation()
    {
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

        if (m_transport) {
            m_transport->Update(0.0f);

            m_state.activeTransportTasks = m_transport->GetActiveTaskCount();
            m_state.blockedTransportTasks = m_transport->GetBlockedCount();
        }

        // Future tick order:
        // 1. EconomySystem — generate demands
        // 2. TransportSystem — move cargo (already done above)
        // 3. ConstructionSystem — process building
        // 4. WorkerSystem — process worker tasks
    }

    const SimulationState& Simulation::GetState() const
    {
        return m_state;
    }

} // namespace World
