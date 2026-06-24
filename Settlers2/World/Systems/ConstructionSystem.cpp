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
    : m_flagManager(NULL)
    , m_roadManager(NULL)
    , m_demandManager(NULL)
    , m_cargoManager(NULL)
    , m_warehouseFlag(NULL)
    , m_eventBus(NULL)
    , m_initialized(false)
{
    m_completed.reserve(16);
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
    m_flagManager = flagManager;
    m_roadManager = roadManager;
    m_demandManager = demandManager;
    m_cargoManager = cargoManager;
    m_warehouseFlag = warehouseFlag;
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
    if (!m_initialized) return;
    if (cmd.type == Building_None) return;
    if (!m_flagManager) return;

    // Determine the entrance flag
    Flag* flag = cmd.entranceFlag;
    if (!flag) {
        // Create a new flag at the building's entrance position.
        // Default entrance offset (1,0) — the caller should provide
        // the correct flag if more precision is needed.
        int entranceX = cmd.tileX + 1;
        int entranceY = cmd.tileY;

        flag = m_flagManager->CreateFlag(entranceX, entranceY);
        if (!flag) return;
        flag->type = FLAG_NORMAL;
        flag->pendingBuilding = cmd.type;
        flag->hasBuilding = true;
    }

    // Mark builder routes dirty to recalculate paths
    m_manager.MarkBuilderRoutesDirty();

    // Create and register the construction site
    ConstructionSite* site = new ConstructionSite(cmd.tileX, cmd.tileY, cmd.type, flag);
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

    // Phase 1: collect newly completed sites into a local array
    // (never iterate the manager's vector while broadcasting — listeners may modify it)
    const std::vector<ConstructionSite*>& sites = m_manager.GetAllSites();
    std::vector<ConstructionSite*> newlyCompleted;
    newlyCompleted.reserve(8);

    for (size_t i = 0; i < sites.size(); ++i) {
        ConstructionSite* s = sites[i];
        if (!s->IsComplete()) continue;

        // Skip already-reported sites (double-fire guard)
        bool alreadyReported = false;
        for (size_t j = 0; j < m_completed.size(); ++j) {
            if (m_completed[j] == s) {
                alreadyReported = true;
                break;
            }
        }
        if (alreadyReported) continue;

        newlyCompleted.push_back(s);
        m_completed.push_back(s);
    }

    // Phase 2: broadcast events for all newly completed sites
    for (size_t i = 0; i < newlyCompleted.size(); ++i) {
        ConstructionSite* s = newlyCompleted[i];
        if (m_eventBus) {
            Core::ConstructionCompleteData data;
            data.siteX = s->x;
            data.siteY = s->y;
            data.buildingType = (int)s->buildingType;
            data.flagId = s->flag ? s->flag->id : 0;
            m_eventBus->Broadcast(Core::Event_ConstructionComplete, &data);
        }
    }

    // Phase 3: purge stale pointers from m_completed (sites removed by ConfirmConstruction)
    // Manual loop instead of erase-remove idiom for C++03 compatibility.
    {
        size_t writeIdx = 0;
        for (size_t i = 0; i < m_completed.size(); ++i) {
            ConstructionSite* s = m_completed[i];
            // Check if this pointer still exists in the manager's site list
            bool stillAlive = false;
            for (size_t j = 0; j < sites.size(); ++j) {
                if (sites[j] == s) {
                    stillAlive = true;
                    break;
                }
            }
            if (stillAlive) {
                m_completed[writeIdx++] = s;
            }
        }
        m_completed.resize(writeIdx);
    }
}

void ConstructionSystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
