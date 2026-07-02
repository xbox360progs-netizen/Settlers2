#include "stdafx.h"
#include "SettlerPresentationSystem.h"
#include "../../World/CarrierManager.h"
#include "../../World/ConstructionManager.h"
#include "../../World/WorkerManager.h"
#include "../../World/RoadManager.h"
#include "../../World/ConstructionSite.h"
#include "../../World/Flag.h"
#include "../../World/Road.h"
#include "../../World/Cargo.h"
#include "../../Logic/CoordinateSystem.h"
#include <math.h>

namespace Scene {

void SettlerPresentationSystem::SetManagers(
    World::CarrierManager* carrierManager,
    World::ConstructionManager* constructionManager,
    World::WorkerManager* workerManager,
    World::RoadManager* roadManager)
{
    m_carrierManager = carrierManager;
    m_constructionManager = constructionManager;
    m_workerManager = workerManager;
    m_roadManager = roadManager;
}

void SettlerPresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    frame.settlers.clear();
    CollectCarriers(frame.settlers);
    CollectBuilders(frame.settlers);
    CollectWorkers(frame.settlers);
}

void SettlerPresentationSystem::CollectCarriers(std::vector<RenderSettler>& out)
{
    if (!m_carrierManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
        World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
        if (!carrier) continue;

        // Only render carriers in transit states
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

        RenderSettler rs;
        rs.transform.worldX = wx;
        rs.transform.worldY = wy;
        rs.transform.depthLayer = 30020 + tileA.y * 400;
        rs.visual.type = SettlerType_Carrier;
        rs.visual.state = SettlerState_Walking;
        rs.visual.dx = dx;
        rs.visual.dy = dy;
        rs.visual.carrying = (carrier->m_cargo != NULL) ? 1 : 0;
        rs.visual.cargoType = carrier->m_cargo ? carrier->m_cargo->type : World::ResourceType_None;
        rs.visual.buildingType = (uint8_t)-1;
        out.push_back(rs);
    }
}

void SettlerPresentationSystem::CollectBuilders(std::vector<RenderSettler>& out)
{
    if (!m_constructionManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    const std::vector<World::ConstructionSite*>& sites = m_constructionManager->GetAllSites();
    for (size_t si = 0; si < sites.size(); ++si) {
        World::ConstructionSite* site = sites[si];
        if (!site || !site->flag) continue;
        if (site->builderState == World::Builder_None) continue;

        RenderSettler rs;
        rs.visual.type = SettlerType_Builder;
        rs.visual.state = SettlerState_Walking;
        rs.visual.carrying = 0;
        rs.visual.cargoType = World::ResourceType_None;
        rs.visual.buildingType = (uint8_t)site->buildingType;

        if (site->builderState == World::Builder_Walking || site->builderState == World::Builder_Returning) {
            if (site->builderRouteCount < 2) continue;

            uint32_t fromIdx = site->builderRouteIndex;
            uint32_t toIdx = fromIdx + 1;

            if (fromIdx >= site->builderRouteCount - 1) {
                // Arrived at final flag
                size_t lastIdx = site->builderRouteCount - 1;
                World::Flag* f = site->builderRoute[lastIdx];
                coords.NodeTileToWorld(f->pos.x, f->pos.y, rs.transform.worldX, rs.transform.worldY);
                // Default SE sprite for idle builder at flag
                rs.visual.dx = 1;
                rs.visual.dy = 1;
                rs.transform.depthLayer = 30020 + f->pos.y * 400;
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
                    rs.transform.worldX = wx0 + (wx1 - wx0) * frac;
                    rs.transform.worldY = wy0 + (wy1 - wy0) * frac;

                    // FIXED: account for walkDir in direction (was bug in original GameRenderer code)
                    float wd = site->builderWalkDir;
                    rs.visual.dx = (wd > 0.0f) ? (tileB.x - tileA.x) : (tileA.x - tileB.x);
                    rs.visual.dy = (wd > 0.0f) ? (tileB.y - tileA.y) : (tileA.y - tileB.y);
                    rs.transform.depthLayer = 30020 + tileA.y * 400;
                } else {
                    // No road — lerp between flags
                    float wx0, wy0, wx1, wy1;
                    coords.NodeTileToWorld(fromFlag->pos.x, fromFlag->pos.y, wx0, wy0);
                    coords.NodeTileToWorld(toFlag->pos.x, toFlag->pos.y, wx1, wy1);
                    float t = (1.0f > 0.0f) ? site->builderEp / 1.0f : 0.0f;
                    if (t < 0.0f) t = 0.0f;
                    if (t > 1.0f) t = 1.0f;
                    rs.transform.worldX = wx0 + (wx1 - wx0) * t;
                    rs.transform.worldY = wy0 + (wy1 - wy0) * t;
                    rs.visual.dx = toFlag->pos.x - fromFlag->pos.x;
                    rs.visual.dy = toFlag->pos.y - fromFlag->pos.y;
                    rs.transform.depthLayer = 30020 + fromFlag->pos.y * 400;
                }
            }

            if (site->builderState == World::Builder_Returning) {
                rs.visual.state = SettlerState_Walking;
            }

        } else if (site->builderState == World::Builder_Building) {
            coords.NodeTileToWorld(site->x, site->y, rs.transform.worldX, rs.transform.worldY);
            rs.visual.dx = 1;
            rs.visual.dy = 1;
            rs.transform.depthLayer = 30020 + site->y * 400;
            rs.visual.state = SettlerState_Building;
        } else {
            continue;
        }

        out.push_back(rs);
    }
}

void SettlerPresentationSystem::CollectWorkers(std::vector<RenderSettler>& out)
{
    if (!m_workerManager) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int wi = 0; wi < m_workerManager->GetActiveCount(); ++wi) {
        const World::Worker* w = m_workerManager->GetWorkerByActiveIdx(wi);
        if (w->state != World::WorkerState_MovingToJob) continue;

        float tx = w->posX;
        float ty = w->posY;
        coords.NodeTileToWorld(tx, ty, tx, ty);

        RenderSettler rs;
        rs.transform.worldX = tx;
        rs.transform.worldY = ty;
        rs.transform.depthLayer = 30020 + (int)(w->posY + 0.5f) * 400;
        rs.visual.type = SettlerType_Worker;
        rs.visual.state = SettlerState_Walking;
        rs.visual.dx = 1;
        rs.visual.dy = 1;
        rs.visual.carrying = 0;
        rs.visual.cargoType = World::ResourceType_None;
        rs.visual.buildingType = (uint8_t)w->profession;
        out.push_back(rs);
    }

    // NOTE: Building worker sprites (workers at their building) are resolved
    // directly by SettlerRenderer from buildingType. No query to Building needed
    // — the renderer knows sprite → buildingType mapping.
}

} // namespace Scene
