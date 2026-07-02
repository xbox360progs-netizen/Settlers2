#include "stdafx.h"
#include "BuildingPresentationSystem.h"
#include "../../World/FlagManager.h"
#include "../../World/ConstructionManager.h"
#include "../../World/ConstructionSite.h"
#include "../../World/Flag.h"
#include "../../World/Components/Building.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void BuildingPresentationSystem::SetManagers(
    World::FlagManager* flagManager,
    World::ConstructionManager* constructionManager)
{
    m_flagManager = flagManager;
    m_constructionManager = constructionManager;
}

void BuildingPresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    frame.buildings.clear();
    CollectFlags(frame.buildings);
    CollectBuildings(frame.buildings);
    CollectConstructionSites(frame.buildings);
}

void BuildingPresentationSystem::CollectFlags(std::vector<RenderBuilding>& out)
{
    if (!m_flagManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    const std::vector<std::pair<int,int> >& pairs = m_flagManager->GetFlagPairs();
    for (size_t i = 0; i < pairs.size(); ++i) {
        int fx = pairs[i].first;
        int fy = pairs[i].second;

        float wx, wy;
        coords.NodeTileToWorld(fx, fy, wx, wy);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + fy * 400;
        rb.visual.kind = 0;
        rb.visual.buildingType = 0;
        rb.visual.depleted = false;
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

void BuildingPresentationSystem::CollectBuildings(std::vector<RenderBuilding>& out)
{
    if (!m_flagManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    const std::vector<std::pair<int,int> >& pairs = m_flagManager->GetFlagPairs();
    for (size_t i = 0; i < pairs.size(); ++i) {
        int fx = pairs[i].first;
        int fy = pairs[i].second;

        World::Flag* flag = m_flagManager->GetFlagAt(fx, fy);
        if (!flag || !flag->building) continue;

        float wx, wy;
        coords.NodeTileToWorld(fx, fy, wx, wy);

        // Check if a worker is present at this building
        float dummyX, dummyY;
        int dummySprite;
        bool hasWorker = flag->building->GetWorkerRenderInfo(dummyX, dummyY, dummySprite);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + fy * 400;
        rb.visual.kind = 1;
        rb.visual.buildingType = static_cast<uint8_t>(flag->building->type);
        rb.visual.depleted = flag->building->IsDepleted();
        rb.visual.fsmState = static_cast<uint8_t>(flag->building->GetFsmState());
        rb.visual.hasWorker = hasWorker;
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

void BuildingPresentationSystem::CollectConstructionSites(std::vector<RenderBuilding>& out)
{
    if (!m_constructionManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
    for (size_t si = 0; si < sites.size(); ++si) {
        World::ConstructionSite* site = sites[si];
        if (!site) continue;

        float wx, wy;
        coords.NodeTileToWorld(site->x, site->y, wx, wy);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + site->y * 400;
        rb.visual.kind = 2;
        rb.visual.buildingType = static_cast<uint8_t>(site->buildingType);
        rb.visual.depleted = false;
        rb.visual.fsmState = 0;
        rb.visual.hasWorker = (site->builderState != World::Builder_None);
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

} // namespace Scene
