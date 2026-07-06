#include "stdafx.h"
#include "CarrierPresentation.h"
#include "../Workers/RenderWorker.h"
#include "../Presentation/Migration/CarrierView.h"
#include "../../Logic/CoordinateSystem.h"
#include <math.h>

namespace Scene {

CarrierPresentation::CarrierPresentation()
    : m_source(NULL)
{
}

void CarrierPresentation::SetCarrierSource(ICarrierSource* source)
{
    m_source = source;
}

void CarrierPresentation::BuildRenderFrame(RenderFrame& frame)
{
    CollectCarriers(frame.workers);
}

void CarrierPresentation::CollectCarriers(std::vector<RenderWorker>& out)
{
    if (!m_source) return;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    uint32_t count = m_source->GetCarrierCount();
    for (uint32_t ci = 0; ci < count; ++ci) {
        CarrierView cv;
        if (!m_source->GetCarrier(ci, cv)) continue;

        // Only render carriers in transit states (WalkingToPost or ReturningHome).
        // Phase 7 carriers in Working state have no route-based movement on the
        // new pipeline — they render as idle until Milestone 4.
        if (cv.state != 0 && cv.state != 2) continue;
        // state 0 = WalkingToPost, 1 = Working (skip), 2 = ReturningHome
        bool inTransit = (cv.state == 0 || cv.state == 2);

        const Vector2i* pathTiles = NULL;
        int pathCount = 0;
        float ep = 0.0f;
        float walkDir = cv.walkDir;

        if (inTransit) {
            if (cv.transitCount < 2) continue;
            pathTiles = cv.transitTiles;
            pathCount = static_cast<int>(cv.transitCount);
            ep = cv.transitProgress;
        } else {
            if (!cv.roadTiles || cv.roadTileCount < 2) continue;
            pathTiles = cv.roadTiles;
            pathCount = static_cast<int>(cv.roadTileCount);
            ep = cv.roadEp;
        }

        int pathLen = pathCount - 1;
        if (ep < 0.0f) ep = 0.0f;
        if (ep > static_cast<float>(pathLen)) ep = static_cast<float>(pathLen);
        int idx = static_cast<int>(ep);
        float frac = ep - static_cast<float>(idx);
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
        rw.carrying = cv.cargoPresent ? 1 : 0;
        rw.cargoType = cv.cargoType;
        rw.buildingType = 255;
        rw.animationFrame = 0;
        out.push_back(rw);
    }
}

} // namespace Scene
