#include "stdafx.h"
#include "BuildingWorkerPresentation.h"
#include "RenderWorker.h"
#include "../Presentation/Migration/IBuildingSource.h"
#include "../Presentation/Migration/BuildingView.h"
#include "../../Logic/CoordinateSystem.h"
#include <math.h>

namespace Scene {

BuildingWorkerPresentation::BuildingWorkerPresentation()
    : m_buildingSource(NULL)
    , m_constructionSiteSource(NULL)
{
}

void BuildingWorkerPresentation::SetSources(
    IBuildingSource* buildingSource,
    IConstructionSiteSource* constructionSiteSource)
{
    m_buildingSource = buildingSource;
    m_constructionSiteSource = constructionSiteSource;
}

void BuildingWorkerPresentation::BuildRenderFrame(RenderFrame& frame)
{
    frame.workers.clear();
    CollectBuildingWorkers(frame.workers);
    CollectBuilders(frame.workers);
}

void BuildingWorkerPresentation::CollectBuildingWorkers(std::vector<RenderWorker>& out)
{
    if (!m_buildingSource) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_buildingSource->GetBuildingCount();
    for (uint32_t i = 0; i < count; ++i) {
        BuildingView bv;
        if (!m_buildingSource->GetBuilding(i, bv)) continue;
        if (bv.workerVisualState == WVS_None) continue;

        float wx, wy;
        coords.NodeTileToWorld(bv.flagX, bv.flagY, wx, wy);

        RenderWorker rw;
        rw.transform.worldX = wx;
        rw.transform.worldY = wy;
        rw.transform.depthLayer = 30020 + bv.flagY * 400 + 1;

        rw.type = 3; // SettlerType_BuildingWorker
        rw.dx = 1;
        rw.dy = 1;
        rw.carrying = 0;
        rw.cargoType = 0;
        rw.buildingType = bv.buildingType;
        rw.animationFrame = 0;

        switch (bv.workerVisualState) {
            case WVS_Working:       rw.state = 2; break;
            case WVS_WalkingToNode:
            case WVS_WalkingToBuilding:
            case WVS_Idle:
            default:                rw.state = 1; break;
        }

        out.push_back(rw);
    }
}

void BuildingWorkerPresentation::CollectBuilders(std::vector<RenderWorker>& out)
{
    if (!m_constructionSiteSource) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_constructionSiteSource->GetConstructionSiteCount();
    for (uint32_t i = 0; i < count; ++i) {
        BuildingView bv;
        if (!m_constructionSiteSource->GetConstructionSite(i, bv)) continue;
        if (bv.workerVisualState == WVS_None) continue;

        float wx, wy;
        coords.NodeTileToWorld(bv.flagX, bv.flagY, wx, wy);

        RenderWorker rw;
        rw.transform.worldX = wx;
        rw.transform.worldY = wy;
        rw.transform.depthLayer = 30020 + bv.flagY * 400;

        rw.type = 1; // SettlerType_Builder
        rw.dx = 1;
        rw.dy = 1;
        rw.carrying = 0;
        rw.cargoType = 0;
        rw.buildingType = bv.buildingType;
        rw.animationFrame = 0;
        rw.state = 3; // SettlerState_Building

        out.push_back(rw);
    }
}

} // namespace Scene
