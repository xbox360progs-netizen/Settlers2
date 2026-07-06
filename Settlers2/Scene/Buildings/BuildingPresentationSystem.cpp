#include "stdafx.h"
#include "BuildingPresentationSystem.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void BuildingPresentationSystem::SetSources(
    IFlagSource* flagSource,
    IBuildingSource* buildingSource,
    IConstructionSiteSource* constructionSiteSource)
{
    m_flagSource = flagSource;
    m_buildingSource = buildingSource;
    m_constructionSiteSource = constructionSiteSource;
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
    if (!m_flagSource) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_flagSource->GetFlagCount();
    for (uint32_t i = 0; i < count; ++i) {
        FlagView fv;
        if (!m_flagSource->GetFlag(i, fv)) continue;

        float wx, wy;
        coords.NodeTileToWorld(fv.nodeX, fv.nodeY, wx, wy);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + fv.nodeY * 400;
        rb.visual.kind = 0;
        rb.visual.buildingType = 0;
        rb.visual.depleted = false;
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

void BuildingPresentationSystem::CollectBuildings(std::vector<RenderBuilding>& out)
{
    if (!m_buildingSource) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_buildingSource->GetBuildingCount();
    for (uint32_t i = 0; i < count; ++i) {
        BuildingView bv;
        if (!m_buildingSource->GetBuilding(i, bv)) continue;

        float wx, wy;
        coords.NodeTileToWorld(bv.flagX, bv.flagY, wx, wy);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + bv.flagY * 400;
        rb.visual.kind = 1;
        rb.visual.buildingType = bv.buildingType;
        rb.visual.depleted = bv.depleted;
        rb.visual.fsmState = bv.fsmState;
        rb.visual.hasWorker = bv.hasWorker;
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

void BuildingPresentationSystem::CollectConstructionSites(std::vector<RenderBuilding>& out)
{
    if (!m_constructionSiteSource) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_constructionSiteSource->GetConstructionSiteCount();
    for (uint32_t i = 0; i < count; ++i) {
        BuildingView bv;
        if (!m_constructionSiteSource->GetConstructionSite(i, bv)) continue;

        float wx, wy;
        coords.NodeTileToWorld(bv.flagX, bv.flagY, wx, wy);

        RenderBuilding rb;
        rb.transform.worldX = wx;
        rb.transform.worldY = wy;
        rb.transform.depthLayer = 30010 + bv.flagY * 400;
        rb.visual.kind = 2;
        rb.visual.buildingType = bv.buildingType;
        rb.visual.depleted = false;
        rb.visual.fsmState = 0;
        rb.visual.hasWorker = bv.hasWorker;
        rb.visual.color = 0xFFFFFFFF;
        out.push_back(rb);
    }
}

} // namespace Scene
