#include "stdafx.h"
#include "ConstructionSystem.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../CarrierManager.h"
#include "../DemandManager.h"
#include "../ConstructionSite.h"
#include "../Flag.h"
#include "../Map.h"
#include "../Road.h"
#include "../../Logic/EconomyManager.h"
#include "../../Core/EventBus.h"
#include "../../Core/CommandBus.h"

namespace World {

ConstructionSystem::ConstructionSystem()
    : m_factory(NULL)
    , m_eventBus(NULL)
    , m_commandBus(NULL)
    , m_carriers(NULL)
    , m_map(NULL)
    , m_initialized(false)
{
    m_completedIds.reserve(16);
}

ConstructionSystem::~ConstructionSystem()
{
    if (m_eventBus) {
        m_eventBus->UnregisterAll(this);
    }
    if (m_commandBus) {
        m_commandBus->UnregisterAll(this);
    }
}

void ConstructionSystem::Initialize(const BuildContext& ctx, Core::EventBus* eventBus, Core::CommandBus* commandBus)
{
    m_factory.SetFlagManager(ctx.flags);
    m_manager.SetFlagManager(ctx.flags);
    m_manager.SetRoadManager(ctx.roads);
    m_manager.SetDemandManager(ctx.demand);
    m_manager.SetWarehouseFlag(ctx.warehouse);
    m_eventBus = eventBus;
    m_commandBus = commandBus;
    m_carriers = ctx.carriers;
    m_map = ctx.map;
    m_relinker.SetManagers(ctx.map, ctx.flags, ctx.roads, ctx.carriers);

    if (m_eventBus) {
        m_eventBus->Register(Core::Event_FlagTopologyChanged, this);
    }
    if (m_commandBus) {
        m_commandBus->Register(Core::Cmd_PlaceFlag, this);
        m_commandBus->Register(Core::Cmd_RemoveConstructionSite, this);
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
            data.siteId = s->id;
            m_eventBus->Post(Core::Event_ConstructionComplete, data);
        }
    }

    // Phase 3: purge stale IDs from m_completedIds (sites removed by BuildingSystem)
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

void ConstructionSystem::HandlePlaceFlag(const Core::PlaceFlagCmd& cmd)
{
    FlagManager* flagManager = m_manager.GetFlagManager();
    RoadManager* roadManager = m_manager.GetRoadManager();
    if (!flagManager || !roadManager) return;

    Flag* flag = flagManager->CreateFlag(cmd.tileX, cmd.tileY);
    if (!flag) return;
    flag->type = World::FLAG_NORMAL;

    if (cmd.isFreeFlag) {
        flag->pendingBuilding = World::Building_None;
        flag->hasBuilding = false;
    } else {
        flag->pendingBuilding = static_cast<BuildingType>(cmd.buildingType);
        flag->hasBuilding = true;
    }

    // Split any road that passes through this flag position
    SplitRoadAtFlag(flag);

    m_relinker.RebuildFromFlag(flag);
    m_relinker.SyncCarriers(flag, m_manager.GetWarehouseFlag());

    // For non-free flags, enqueue construction
    if (!cmd.isFreeFlag && cmd.buildingType != World::Building_None) {
        BuildCommand bcmd;
        bcmd.type = static_cast<BuildingType>(cmd.buildingType);
        bcmd.tileX = cmd.buildX;
        bcmd.tileY = cmd.buildY;
        bcmd.entranceFlag = flag;
        bcmd.autoConnectRoad = cmd.autoConnectRoad;
        Enqueue(bcmd);
    }

    // Post Event_FlagPlaced
    if (m_eventBus) {
        Core::FlagPlacedData fd;
        fd.flagId = flag->id;
        fd.posX = flag->pos.x;
        fd.posY = flag->pos.y;
        fd.buildingType = cmd.isFreeFlag ? (int)World::Building_None : cmd.buildingType;
        fd.buildX = cmd.buildX;
        fd.buildY = cmd.buildY;
        m_eventBus->Post(Core::Event_FlagPlaced, fd);
    }
}

void ConstructionSystem::SplitRoadAtFlag(Flag* flag)
{
    RoadManager* roadManager = m_manager.GetRoadManager();
    FlagManager* flagManager = m_manager.GetFlagManager();
    if (!flag || !roadManager || !m_carriers) return;

    size_t roadCount = roadManager->GetCount();
    for (size_t ri = 0; ri < roadCount; ++ri) {
        Road* road = roadManager->GetRoad(ri);
        if (!road) continue;

        int splitIdx = -1;
        for (uint32_t t = 1; t + 1 < road->tileCount; ++t) {
            if (road->tiles[t].x == flag->pos.x && road->tiles[t].y == flag->pos.y) {
                splitIdx = (int)t;
                break;
            }
        }
        if (splitIdx < 0) continue;

        Flag* ra = flagManager ? flagManager->ResolveFlag(road->a) : NULL;
        Flag* rb = flagManager ? flagManager->ResolveFlag(road->b) : NULL;
        if (!ra || !rb) continue;

        // Build two tile paths: A->X and X->B
        std::vector<Vector2i> pathAX, pathXB;
        for (uint32_t t = 0; t <= (uint32_t)splitIdx; ++t)
            pathAX.push_back(road->tiles[t]);
        for (uint32_t t = (uint32_t)splitIdx; t < road->tileCount; ++t)
            pathXB.push_back(road->tiles[t]);

        // Remove old road
        if (m_carriers) m_carriers->RemoveCarriersForRoad(road);
        m_manager.OnRoadRemoved(road);
        roadManager->RemoveRoad(road);

        // Create two new roads
        Road* ax = roadManager->CreateRoad(ra, flag, pathAX);
        Road* xb = roadManager->CreateRoad(flag, rb, pathXB);

        // Sync carriers for new segments only
        if (ax && m_carriers) m_carriers->SyncCarriersForRoad(ax);
        if (xb && m_carriers) m_carriers->SyncCarriersForRoad(xb);

        return; // only one road can pass through a given position
    }
}

void ConstructionSystem::OnEvent(Core::EventType type, void* data)
{
    if (type == Core::Event_FlagTopologyChanged) {
        m_manager.MarkBuilderRoutesDirty();
    }
}

void ConstructionSystem::OnCommand(Core::CommandType type, void* data)
{
    if (type == Core::Cmd_PlaceFlag) {
        Core::PlaceFlagCmd* cmd = static_cast<Core::PlaceFlagCmd*>(data);
        HandlePlaceFlag(*cmd);
    } else if (type == Core::Cmd_RemoveConstructionSite) {
        Core::RemoveConstructionSiteCmd* cmd = static_cast<Core::RemoveConstructionSiteCmd*>(data);
        const std::vector<ConstructionSite*>& sites = m_manager.GetAllSites();
        for (size_t i = 0; i < sites.size(); ++i) {
            if (sites[i]->id == cmd->siteId) {
                m_manager.RemoveSite(sites[i]);
                break;
            }
        }
    }
}

} // namespace World
