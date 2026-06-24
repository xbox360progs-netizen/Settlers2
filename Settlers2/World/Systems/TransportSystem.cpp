#include "stdafx.h"
#include "TransportSystem.h"
#include "../CarrierManager.h"
#include "../CarrierSystem.h"
#include "../TransportJobManager.h"
#include "../CargoManager.h"
#include "../DemandManager.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../EntityManager.h"
#include "../../Core/EventBus.h"

namespace World {

TransportSystem::TransportSystem()
    : m_carrierManager(NULL)
    , m_carrierSystem(NULL)
    , m_transportJobManager(NULL)
    , m_cargoManager(NULL)
    , m_demandManager(NULL)
    , m_flagManager(NULL)
    , m_roadManager(NULL)
    , m_eventBus(NULL)
    , m_ownsManagers(false)
    , m_externalMode(false)
    , m_initialized(false)
{
}

TransportSystem::~TransportSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }

    if (m_ownsManagers && !m_externalMode) {
        if (m_cargoManager) { delete m_cargoManager; m_cargoManager = NULL; }
        if (m_demandManager) { delete m_demandManager; m_demandManager = NULL; }
        if (m_transportJobManager) { delete m_transportJobManager; m_transportJobManager = NULL; }
        if (m_carrierManager) { delete m_carrierManager; m_carrierManager = NULL; }
        if (m_carrierSystem) { delete m_carrierSystem; m_carrierSystem = NULL; }
    }
}

void TransportSystem::SetExternalManagers(
    CarrierManager* carriers,
    CarrierSystem* carrierSystem,
    TransportJobManager* transportJobs,
    CargoManager* cargo,
    DemandManager* demand,
    FlagManager* flagManager,
    RoadManager* roadManager)
{
    m_carrierManager = carriers;
    m_carrierSystem = carrierSystem;
    m_transportJobManager = transportJobs;
    m_cargoManager = cargo;
    m_demandManager = demand;
    m_flagManager = flagManager;
    m_roadManager = roadManager;
    m_externalMode = true;
}

void TransportSystem::Initialize(
    EntityManager* entityManager,
    FlagManager* flagManager,
    RoadManager* roadManager,
    Core::EventBus* eventBus)
{
    m_eventBus = eventBus;

    if (!m_externalMode) {
        m_flagManager = flagManager;
        m_roadManager = roadManager;

        m_carrierSystem = new CarrierSystem(entityManager);
        m_carrierManager = new CarrierManager();
        m_carrierManager->SetCarrierSystem(m_carrierSystem);

        m_transportJobManager = new TransportJobManager();
        m_cargoManager = new CargoManager();
        m_demandManager = new DemandManager();

        m_ownsManagers = true;

        m_carrierManager->SetFlagManager(m_flagManager);
        m_carrierManager->SetRoadManager(m_roadManager);
        m_carrierManager->SetJobManager(m_transportJobManager);
        m_carrierManager->SetCargoManager(m_cargoManager);
        m_carrierManager->SetDemandManager(m_demandManager);

        m_transportJobManager->SetFlagManager(m_flagManager);
        m_transportJobManager->SetRoadManager(m_roadManager);
        m_transportJobManager->SetCarrierManager(m_carrierManager);
    }

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_FlagPlaced, this);
        m_eventBus->Register(Core::Event_RoadBuilt, this);
        m_eventBus->Register(Core::Event_FlagDeleted, this);
    }

    m_initialized = true;
}

void TransportSystem::Update(float dt)
{
    if (!m_initialized) return;
    if (m_carrierManager) m_carrierManager->Update(dt);
}

void TransportSystem::SetWarehouseFlag(Flag* flag)
{
    if (m_carrierManager) m_carrierManager->SetWarehouseFlag(flag);
}

void TransportSystem::SyncCarriersForFlag(Flag* flag)
{
    (void)flag;
}

int TransportSystem::GetCarrierCount() const
{
    return m_carrierManager ? m_carrierManager->GetCarrierCount() : 0;
}

void TransportSystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
