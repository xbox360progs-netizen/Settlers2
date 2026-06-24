#include "stdafx.h"
#include "ConstructionSystem.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../DemandManager.h"
#include "../ConstructionSite.h"
#include "../../Logic/EconomyManager.h"
#include "../../Core/EventBus.h"

namespace World {

ConstructionSystem::ConstructionSystem()
    : m_eventBus(NULL)
    , m_initialized(false)
{
}

ConstructionSystem::~ConstructionSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
}

void ConstructionSystem::Initialize(
    FlagManager* flagManager,
    RoadManager* roadManager,
    DemandManager* demandManager,
    CargoManager* cargoManager,
    Flag* warehouseFlag,
    Core::EventBus* eventBus)
{
    m_manager.SetFlagManager(flagManager);
    m_manager.SetRoadManager(roadManager);
    m_manager.SetDemandManager(demandManager);
    m_manager.SetWarehouseFlag(warehouseFlag);
    m_eventBus = eventBus;

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_FlagDeleted, this);
    }

    m_initialized = true;
}

void ConstructionSystem::Enqueue(const BuildCommand& cmd)
{
    if (!cmd.valid) return;
    if (!m_initialized) return;

    Flag* flag = m_manager.GetFlagManager()
        ? m_manager.GetFlagManager()->GetFlagById(cmd.flagId)
        : NULL;
    if (!flag) return;

    ConstructionSite* site = new ConstructionSite(cmd.posX, cmd.posY, cmd.buildingType, flag);
    m_manager.AddSite(site);
}

void ConstructionSystem::Update(float dt)
{
    if (!m_initialized) return;
    m_manager.Update(dt);
}

void ConstructionSystem::GenerateRequests(Logic::EconomyManager* economy)
{
    if (!m_initialized) return;
    m_manager.GenerateRequests(economy);
}

void ConstructionSystem::OnRoadRemoved(Road* road)
{
    if (!m_initialized) return;
    m_manager.OnRoadRemoved(road);
}

ConstructionSite* ConstructionSystem::GetSiteAt(int x, int y) const
{
    return m_manager.GetSiteAt(x, y);
}

ConstructionSite* ConstructionSystem::GetSiteForFlag(const Flag* flag) const
{
    return m_manager.GetSiteForFlag(flag);
}

Flag* ConstructionSystem::FindConstructionDemand(Flag* fromFlag, ResourceType type) const
{
    return m_manager.FindConstructionDemand(fromFlag, type);
}

void ConstructionSystem::PostUpdate()
{
    if (!m_initialized) return;

    // Scan sites for completions and broadcast events
    const std::vector<ConstructionSite*>& sites = m_manager.GetAllSites();
    for (size_t i = 0; i < sites.size(); ++i) {
        ConstructionSite* s = sites[i];
        if (!s->IsComplete()) continue;

        // Broadcast construction complete event
        if (m_eventBus) {
            Core::ConstructionCompleteData data;
            data.siteX = s->x;
            data.siteY = s->y;
            data.buildingType = (int)s->buildingType;
            data.flagId = s->flag ? s->flag->id : 0;
            m_eventBus->Broadcast(Core::Event_ConstructionComplete, &data);
        }
    }
}

void ConstructionSystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
