#include "stdafx.h"
#include "EconomySystem.h"
#include "../../Logic/EconomyManager.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../CargoManager.h"
#include "../StorehouseManager.h"
#include "../DemandManager.h"
#include "../Components/Building.h"
#include "../Warehouse.h"
#include "../../Core/EventBus.h"

namespace World {

EconomySystem::EconomySystem()
    : m_manager(NULL)
    , m_ownsManager(false)
    , m_eventBus(NULL)
    , m_demandManager(NULL)
{
}

EconomySystem::~EconomySystem()
{
    if (m_ownsManager && m_manager) {
        delete m_manager;
        m_manager = NULL;
    }
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
}

void EconomySystem::SetManager(Logic::EconomyManager* mgr)
{
    m_manager = mgr;
    m_ownsManager = false;
}

void EconomySystem::Initialize(Core::EventBus* eventBus)
{
    m_eventBus = eventBus;
    if (!m_manager) {
        m_manager = new Logic::EconomyManager();
        m_ownsManager = true;
    }

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_ConstructionComplete, this);
        m_eventBus->Register(Core::Event_FlagDeleted, this);
    }
}

void EconomySystem::SetFlagManager(FlagManager* fm) { if (m_manager) m_manager->SetFlagManager(fm); }
void EconomySystem::SetRoadManager(RoadManager* rm) { if (m_manager) m_manager->SetRoadManager(rm); }
void EconomySystem::SetCargoManager(CargoManager* cm) { if (m_manager) m_manager->SetCargoManager(cm); }
void EconomySystem::SetStorehouseManager(StorehouseManager* sm) { if (m_manager) m_manager->SetStorehouseManager(sm); }
void EconomySystem::SetDemandManager(DemandManager* dm) { m_demandManager = dm; }

void EconomySystem::AddBuilding(Building* building) { if (m_manager) m_manager->AddBuilding(building); }
void EconomySystem::RemoveBuilding(Building* building) { if (m_manager) m_manager->RemoveBuilding(building); }
void EconomySystem::SetWarehouse(Warehouse* warehouse) { if (m_manager) m_manager->SetWarehouse(warehouse); }

Warehouse* EconomySystem::GetWarehouse() const { return m_manager ? m_manager->GetWarehouse() : NULL; }
StorehouseManager* EconomySystem::GetStorehouseManager() const { return m_manager ? m_manager->GetStorehouseManager() : NULL; }

void EconomySystem::RequestResource(Building* requester, int type, int amount, int priority)
{
    if (m_manager) m_manager->RequestResource(requester, static_cast<ResourceType>(type), amount, priority);
}

void EconomySystem::RequestConstructionResource(Flag* destFlag, int type, int amount, int priority)
{
    if (m_manager) m_manager->RequestConstructionResource(destFlag, static_cast<ResourceType>(type), amount, priority);
}

int EconomySystem::GetTotalStock(int type) const
{
    return m_manager ? m_manager->GetTotalStock(static_cast<ResourceType>(type)) : 0;
}

int EconomySystem::GetCargoInTransit(int type) const
{
    return m_manager ? m_manager->GetCargoInTransit(static_cast<ResourceType>(type)) : 0;
}

int EconomySystem::GetCargoOnFlags(int type) const
{
    return m_manager ? m_manager->GetCargoOnFlags(static_cast<ResourceType>(type)) : 0;
}

void EconomySystem::Update(float dt)
{
    if (m_manager) m_manager->Update(dt);
}

void EconomySystem::CollectWarehouse()
{
    if (m_manager) m_manager->CollectWarehouse();
}

void EconomySystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
