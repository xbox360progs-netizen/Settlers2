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
    : m_factory(NULL)
    , m_eventBus(NULL)
    , m_initialized(false)
{
    m_completedIds.reserve(16);
}

ConstructionSystem::~ConstructionSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
}

void ConstructionSystem::Initialize(const BuildContext& ctx, Core::EventBus* eventBus)
{
    m_factory.SetFlagManager(ctx.flags);
    m_manager.SetFlagManager(ctx.flags);
    m_manager.SetRoadManager(ctx.roads);
    m_manager.SetDemandManager(ctx.demand);
    m_manager.SetWarehouseFlag(ctx.warehouse);
    m_eventBus = eventBus;

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_FlagDeleted, this);
    }

    m_initialized = true;
}

void ConstructionSystem::Enqueue(const BuildCommand& cmd)
{
    if (!m_initialized) return;

    ConstructionSite* site = m_factory.Create(cmd);
    if (!site) return;

    m_manager.MarkBuilderRoutesDirty();
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
    // (never broadcast while iterating the manager's vector — listeners may modify it)
    const std::vector<ConstructionSite*>& sites = m_manager.GetAllSites();
    std::vector<ConstructionSite*> newlyCompleted;
    newlyCompleted.reserve(8);

    for (size_t i = 0; i < sites.size(); ++i) {
        ConstructionSite* s = sites[i];
        if (!s->IsComplete()) continue;

        // Skip already-reported sites (double-fire guard using stable ID)
        bool alreadyReported = false;
        for (size_t j = 0; j < m_completedIds.size(); ++j) {
            if (m_completedIds[j] == s->id) {
                alreadyReported = true;
                break;
            }
        }
        if (alreadyReported) continue;

        newlyCompleted.push_back(s);
        m_completedIds.push_back(s->id);
    }

    // Phase 2: post events for all newly completed sites
    // (dispatched by Simulation::Flush in phase 7)
    for (size_t i = 0; i < newlyCompleted.size(); ++i) {
        ConstructionSite* s = newlyCompleted[i];
        if (m_eventBus) {
            Core::ConstructionCompleteData data;
            data.siteX = s->x;
            data.siteY = s->y;
            data.buildingType = (int)s->buildingType;
            data.flagId = s->flag ? s->flag->id : 0;
            m_eventBus->Post(Core::Event_ConstructionComplete, data);
        }
    }

    // Phase 3: purge stale IDs from m_completedIds (sites removed by ConfirmConstruction)
    {
        size_t writeIdx = 0;
        for (size_t i = 0; i < m_completedIds.size(); ++i) {
            ConstructionSiteId pid = m_completedIds[i];
            bool stillAlive = false;
            for (size_t j = 0; j < sites.size(); ++j) {
                if (sites[j]->id == pid) {
                    stillAlive = true;
                    break;
                }
            }
            if (stillAlive) {
                m_completedIds[writeIdx++] = pid;
            }
        }
        m_completedIds.resize(writeIdx);
    }
}

void ConstructionSystem::OnEvent(Core::EventType type, void* data)
{
    (void)data;
    (void)type;
}

} // namespace World
