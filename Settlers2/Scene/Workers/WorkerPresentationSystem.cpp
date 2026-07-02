#include "stdafx.h"
#include "WorkerPresentationSystem.h"
#include "../../World/CarrierManager.h"
#include "../../World/ConstructionManager.h"
#include "../../World/WorkerManager.h"
#include "../../World/FlagManager.h"
#include "../../World/RoadManager.h"
#include "../../World/ConstructionSite.h"
#include "../../World/Flag.h"
#include "../../World/Road.h"
#include "../../World/Cargo.h"
#include "../../World/Components/Building.h"
#include "../../Logic/CoordinateSystem.h"
#include <math.h>

namespace Scene {

WorkerPresentationSystem::WorkerPresentationSystem()
    : m_carrierManager(NULL)
    , m_constructionManager(NULL)
    , m_workerManager(NULL)
    , m_flagManager(NULL)
    , m_roadManager(NULL)
{
}

void WorkerPresentationSystem::SetManagers(
    World::CarrierManager* carrierManager,
    World::ConstructionManager* constructionManager,
    World::WorkerManager* workerManager,
    World::FlagManager* flagManager,
    World::RoadManager* roadManager)
{
    m_carrierManager = carrierManager;
    m_constructionManager = constructionManager;
    m_workerManager = workerManager;
    m_flagManager = flagManager;
    m_roadManager = roadManager;
}

void WorkerPresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    frame.workers.clear();
    CollectCarriers(frame.workers);
    CollectBuilders(frame.workers);
    CollectMovingWorkers(frame.workers);
    CollectBuildingWorkers(frame.workers);
}

void WorkerPresentationSystem::CollectCarriers(std::vector<RenderWorker>& out)
{
    if (!m_carrierManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
        World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
        if (!carrier) continue;

        if (!World::IsTransitState(carrier->state)) continue;

        const Vector2i* pathTiles = NULL;
        int pathCount = 0;
        float ep = 0.0f;
        float walkDir = carrier->walkDir;

        if (World::IsTransitState(carrier->state)) {
            if (carrier->transitCount < 2) continue;
            pathTiles = carrier->transitTiles;
            pathCount = (int)carrier->transitCount;
            ep = carrier->transitProgress;
        } else {
            if (!carrier->road || carrier->road->tileCount < 2) continue;
            pathTiles = carrier->road->tiles;
            pathCount = (int)carrier->road->tileCount;
            ep = carrier->ep;
        }

        int pathLen = pathCount - 1;
        if (ep < 0.0f) ep = 0.0f;
        if (ep > (float)pathLen) ep = (float)pathLen;
        int idx = (int)ep;
        float frac = ep - (float)idx;
        if (idx >= pathLen) { idx = pathLen - 1; frac = 1.0f; }
        if (idx < 0) { idx = 0; frac = 0.0f; }

        const Vector2i& tileA = pathTiles[idx];
        const Vector2i& tileB = pathTiles[idx + 1];

        int dx = (walkDir > 0.0f) ? (tileB.x - tileA.x) : (tileA.x - tileB.x);
        int dy = (walkDir > 0.0f) ? (tileB.y - tileA.y) : (tileA.y - tileB.y);

        float wx0, wy0, wx1, wy1;
        coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
        coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
        float wx = wx0 + (wx1 - wx0) * frac;
        float wy = wy0 + (wy1 - wy0) * frac;

        RenderWorker rw;
        rw.transform.worldX = wx;
        rw.transform.worldY = wy;
        rw.transform.depthLayer = 30020 + tileA.y * 400;
        rw.type = 0; // SettlerType_Carrier
        rw.state = 0; // SettlerState_Walking
        rw.dx = static_cast<int8_t>(dx);
        rw.dy = static_cast<int8_t>(dy);
        rw.carrying = (carrier->m_cargo != NULL) ? 1 : 0;
        rw.cargoType = carrier->m_cargo ? static_cast<uint8_t>(carrier->m_cargo->type) : 0;
        rw.buildingType = 255;
        rw.animationFrame = 0;
        out.push_back(rw);
    }
}

void WorkerPresentationSystem::CollectBuilders(std::vector<RenderWorker>& out)
{
    if (!m_constructionManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
    for (size_t si = 0; si < sites.size(); ++si) {
        World::ConstructionSite* site = sites[si];
        if (!site || !site->flag) continue;
        if (site->builderState == World::Builder_None) continue;

        RenderWorker rw;
        rw.type = 1; // SettlerType_Builder
        rw.state = 0; // SettlerState_Walking
        rw.carrying = 0;
        rw.cargoType = 0;
        rw.buildingType = static_cast<uint8_t>(site->buildingType);
        rw.animationFrame = 0;

        if (site->builderState == World::Builder_Walking || site->builderState == World::Builder_Returning) {
            if (site->builderRouteCount < 2) continue;

            uint32_t fromIdx = site->builderRouteIndex;
            uint32_t toIdx = fromIdx + 1;

            if (fromIdx >= site->builderRouteCount - 1) {
                size_t lastIdx = site->builderRouteCount - 1;
                World::Flag* f = site->builderRoute[lastIdx];
                coords.NodeTileToWorld(f->pos.x, f->pos.y, rw.transform.worldX, rw.transform.worldY);
                rw.dx = 1;
                rw.dy = 1;
                rw.transform.depthLayer = 30020 + f->pos.y * 400;
            } else {
                World::Flag* fromFlag = site->builderRoute[fromIdx];
                World::Flag* toFlag = site->builderRoute[toIdx];
                World::Road* road = m_roadManager ? m_roadManager->GetRoadBetween(fromFlag, toFlag) : NULL;

                if (road && road->tileCount >= 2) {
                    int tc = (int)road->tileCount;
                    float pathLen = (float)(tc - 1);
                    float pos = site->builderEp;
                    if (pos < 0.0f) pos = 0.0f;
                    if (pos > pathLen) pos = pathLen;
                    int tileIdx = (int)pos;
                    float frac = pos - (float)tileIdx;
                    if (tileIdx >= tc - 1) { tileIdx = tc - 2; frac = 1.0f; }
                    if (tileIdx < 0) { tileIdx = 0; frac = 0.0f; }

                    const Vector2i& tileA = road->tiles[tileIdx];
                    const Vector2i& tileB = road->tiles[tileIdx + 1];

                    float wx0, wy0, wx1, wy1;
                    coords.NodeTileToWorld(tileA.x, tileA.y, wx0, wy0);
                    coords.NodeTileToWorld(tileB.x, tileB.y, wx1, wy1);
                    rw.transform.worldX = wx0 + (wx1 - wx0) * frac;
                    rw.transform.worldY = wy0 + (wy1 - wy0) * frac;

                    float wd = site->builderWalkDir;
                    rw.dx = static_cast<int8_t>((wd > 0.0f) ? (tileB.x - tileA.x) : (tileA.x - tileB.x));
                    rw.dy = static_cast<int8_t>((wd > 0.0f) ? (tileB.y - tileA.y) : (tileA.y - tileB.y));
                    rw.transform.depthLayer = 30020 + tileA.y * 400;
                } else {
                    float wx0, wy0, wx1, wy1;
                    coords.NodeTileToWorld(fromFlag->pos.x, fromFlag->pos.y, wx0, wy0);
                    coords.NodeTileToWorld(toFlag->pos.x, toFlag->pos.y, wx1, wy1);
                    float t = (1.0f > 0.0f) ? site->builderEp / 1.0f : 0.0f;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                    rw.transform.worldX = wx0 + (wx1 - wx0) * t;
                    rw.transform.worldY = wy0 + (wy1 - wy0) * t;
                    rw.dx = static_cast<int8_t>(toFlag->pos.x - fromFlag->pos.x);
                    rw.dy = static_cast<int8_t>(toFlag->pos.y - fromFlag->pos.y);
                    rw.transform.depthLayer = 30020 + fromFlag->pos.y * 400;
                }
            }

            if (site->builderState == World::Builder_Returning) {
                rw.state = 0; // SettlerState_Walking
            }

        } else if (site->builderState == World::Builder_Building) {
            coords.NodeTileToWorld(site->x, site->y, rw.transform.worldX, rw.transform.worldY);
            rw.dx = 1;
            rw.dy = 1;
            rw.transform.depthLayer = 30020 + site->y * 400;
            rw.state = 3; // SettlerState_Building
        } else {
            continue;
        }

        out.push_back(rw);
    }
}

void WorkerPresentationSystem::CollectMovingWorkers(std::vector<RenderWorker>& out)
{
    if (!m_workerManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int wi = 0; wi < m_workerManager->GetActiveCount(); ++wi) {
        const World::Worker* w = m_workerManager->GetWorkerByActiveIdx(wi);
        if (w->state != World::WorkerState_MovingToJob) continue;

        float tx = w->posX;
        float ty = w->posY;
        coords.NodeTileToWorld(tx, ty, tx, ty);

        RenderWorker rw;
        rw.transform.worldX = tx;
        rw.transform.worldY = ty;
        rw.transform.depthLayer = 30020 + (int)(w->posY + 0.5f) * 400;
        rw.type = 2; // SettlerType_Worker
        rw.state = 0; // SettlerState_Walking
        rw.dx = 1;
        rw.dy = 1;
        rw.carrying = 0;
        rw.cargoType = 0;
        rw.buildingType = static_cast<uint8_t>(w->profession);
        rw.animationFrame = 0;
        out.push_back(rw);
    }
}

void WorkerPresentationSystem::CollectBuildingWorkers(std::vector<RenderWorker>& out)
{
    if (!m_flagManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int fi = 0; fi < (int)m_flagManager->GetCount(); ++fi) {
        World::Flag* flag = m_flagManager->GetFlagByIndex(fi);
        if (!flag || !flag->building) continue;

        float wx, wy;
        int wSpriteIdx;
        if (!flag->building->GetWorkerRenderInfo(wx, wy, wSpriteIdx)) continue;

        float wwx, wwy;
        coords.NodeTileToWorld(wx, wy, wwx, wwy);

        RenderWorker rw;
        rw.transform.worldX = wwx;
        rw.transform.worldY = wwy;
        rw.transform.depthLayer = 30020 + (int)(wy + 0.5f) * 400 + 1;

        rw.type = 3; // SettlerType_BuildingWorker
        rw.state = 1; // SettlerState_Idle
        rw.dx = 1;
        rw.dy = 1;
        rw.carrying = 0;
        rw.cargoType = 0;
        rw.buildingType = static_cast<uint8_t>(flag->building->type);
        rw.animationFrame = 0;
        out.push_back(rw);
    }
}

} // namespace Scene
