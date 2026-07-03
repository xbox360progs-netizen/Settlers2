#include "stdafx.h"
#include "LogisticsDebugPresentationSystem.h"
#include "RenderDebugLabel.h"
#include "../FrameContext.h"
#include "../../World/FlagManager.h"
#include "../../World/Flag.h"
#include "../../World/CarrierManager.h"
#include "../../World/Carrier.h"
#include "../../World/Components/Building.h"
#include "../../Logic/CoordinateSystem.h"
#include <cstdio>

namespace Scene {

void LogisticsDebugPresentationSystem::SetManagers(
    World::FlagManager* flagManager,
    World::CarrierManager* carrierManager)
{
    m_flagManager = flagManager;
    m_carrierManager = carrierManager;
}

void LogisticsDebugPresentationSystem::BuildRenderFrame(
    const FrameContext& frame,
    std::vector<RenderDebugLabel>& labels)
{
    if (!frame.input.logisticsDebug) return;

    labels.clear();

    if (!m_flagManager && !m_carrierManager) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    // ─── Flag labels ────────────────────────────────────────────────
    if (m_flagManager) {
        char buf[64];
        for (size_t fi = 0; fi < m_flagManager->GetCount(); ++fi) {
            World::Flag* flag = m_flagManager->GetFlag(fi);
            if (!flag) continue;

            float wx, wy;
            coords.NodeTileToWorld(flag->pos.x, flag->pos.y, wx, wy);

            buf[0] = '\0';
            int avail = 0;
            for (int si = 0; si < 8; ++si) {
                if (flag->slots[si].type != World::ResourceType_None && flag->slots[si].amount > 0) {
                    char tag[8];
                    switch (flag->slots[si].type) {
                        case World::ResourceType_Wood:   _snprintf(tag, sizeof(tag), "W:%d ", flag->slots[si].amount); break;
                        case World::ResourceType_Planks: _snprintf(tag, sizeof(tag), "P:%d ", flag->slots[si].amount); break;
                        case World::ResourceType_Stone:  _snprintf(tag, sizeof(tag), "S:%d ", flag->slots[si].amount); break;
                        case World::ResourceType_Fish:   _snprintf(tag, sizeof(tag), "F:%d ", flag->slots[si].amount); break;
                        case World::ResourceType_Meat:   _snprintf(tag, sizeof(tag), "M:%d ", flag->slots[si].amount); break;
                        case World::ResourceType_Coal:   _snprintf(tag, sizeof(tag), "C:%d ", flag->slots[si].amount); break;
                        default:
                            _snprintf(tag, sizeof(tag), "%s:%d ",
                                World::ResourceTypeToString(flag->slots[si].type), flag->slots[si].amount);
                            break;
                    }
                    int written = _snprintf(buf + avail, sizeof(buf) - avail, "%s", tag);
                    if (written > 0) avail += written;
                    if (avail >= static_cast<int>(sizeof(buf)) - 2) break;
                }
            }
            _snprintf(buf + avail, sizeof(buf) - avail, "~%d", flag->id);

            float ty = wy + 12.0f;
            if (flag->hasBuilding) ty += 20.0f;

            RenderDebugLabel label;
            label.worldX = wx - 30.0f;
            label.worldY = ty;
            label.color = D3DCOLOR_ARGB(220, 255, 255, 200);
            label.scale = 0.06f;
            label.fontId = 1;  // FONT_DEBUG
            label.style = 0;   // FONT_STYLE_NORMAL
            label.depth = 0.05f;
            label.layer = 0;  // LAYER_EFFECTS
            strncpy_s(label.text, sizeof(label.text), buf, _TRUNCATE);
            labels.push_back(label);
        }
    }

    // ─── Carrier labels ─────────────────────────────────────────────
    if (m_carrierManager) {
        char buf[64];
        for (int ci = 0; ci < m_carrierManager->GetCarrierCount(); ++ci) {
            World::Carrier* carrier = m_carrierManager->GetCarrier(ci);
            if (!carrier) continue;

            const Vector2i* pathTiles = NULL;
            int pathCount = 0;
            float ep = 0.0f;

            if (World::IsTransitState(carrier->state)) {
                if (carrier->transitCount < 2) continue;
                pathTiles = carrier->transitTiles;
                pathCount = static_cast<int>(carrier->transitCount);
                ep = carrier->transitProgress;
            } else {
                if (!carrier->road || carrier->road->tileCount < 2) continue;
                pathTiles = carrier->road->tiles;
                pathCount = static_cast<int>(carrier->road->tileCount);
                ep = carrier->ep;
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

            float cx, cy, nx, ny;
            coords.NodeTileToWorld(tileA.x, tileA.y, cx, cy);
            coords.NodeTileToWorld(tileB.x, tileB.y, nx, ny);
            float wx = cx + (nx - cx) * frac;
            float wy = cy + (ny - cy) * frac;

            const char* cargoName = "Idle";
            if (carrier->m_cargo) {
                cargoName = World::ResourceTypeToString(carrier->m_cargo->type);
            }

            if (carrier->road) {
                _snprintf(buf, sizeof(buf), "%s %u<->%u", cargoName,
                    carrier->m_roadEndpointA ? carrier->m_roadEndpointA->id : 0,
                    carrier->m_roadEndpointB ? carrier->m_roadEndpointB->id : 0);
            } else {
                _snprintf(buf, sizeof(buf), "%s (transit)", cargoName);
            }

            RenderDebugLabel label;
            label.worldX = wx - 20.0f;
            label.worldY = wy - 20.0f;
            label.color = D3DCOLOR_ARGB(220, 200, 255, 200);
            label.scale = 0.05f;
            label.fontId = 1;   // FONT_DEBUG
            label.style = 0;    // FONT_STYLE_NORMAL
            label.depth = 0.05f;
            label.layer = 0;    // LAYER_EFFECTS
            strncpy_s(label.text, sizeof(label.text), buf, _TRUNCATE);
            labels.push_back(label);
        }
    }

    // ─── Header label ───────────────────────────────────────────────
    {
        RenderDebugLabel header;
        header.worldX = 10.0f;
        header.worldY = 10.0f;
        header.color = D3DCOLOR_ARGB(180, 255, 255, 255);
        header.scale = 0.08f;
        header.fontId = 0;   // FONT_MENU
        header.style = 0;
        header.depth = 0.05f;
        header.layer = 0;
        strncpy_s(header.text, sizeof(header.text),
            "LOGISTICS DEBUG ON (Back=toggle)", _TRUNCATE);
        header.isScreenSpace = true;  // screen coords
        labels.push_back(header);
    }
}

} // namespace Scene
