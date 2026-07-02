#include "stdafx.h"
#include "GeologistOverlayPresentationSystem.h"
#include "../../World/Map.h"
#include "../../World/ResourceNode.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void GeologistOverlayPresentationSystem::BuildRenderFrame(
    const FrameContext& frame,
    std::vector<RenderOverlayMarker>& out)
{
    if (!m_map) return;

    CollectMountainHighlight(frame, out);
    CollectSurveyedDeposits(out);
    CollectWorkingIndicator(frame, out);
}

void GeologistOverlayPresentationSystem::CollectMountainHighlight(
    const FrameContext& frame,
    std::vector<RenderOverlayMarker>& out)
{
    int cx = frame.input.cursorTileX;
    int cy = frame.input.cursorTileY;
    if (cx < 0 || cy < 0) return;

    const World::Tile& objTile = m_map->GetTile(World::Objects, cx, cy);
    if (objTile.type != World::Mountain &&
        objTile.type != World::MountainOnWater &&
        objTile.type != World::Rock)
        return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    float wx, wy;
    coords.NodeTileToWorld(cx, cy, wx, wy);

    RenderOverlayMarker marker;
    marker.markerType = OVERLAY_MARKER_MOUNTAIN;
    marker.transform.worldX = wx;
    marker.transform.worldY = wy;
    marker.transform.depthLayer = static_cast<int>(0.98f * 65535.0f);
    out.push_back(marker);
}

void GeologistOverlayPresentationSystem::CollectSurveyedDeposits(
    std::vector<RenderOverlayMarker>& out)
{
    int w = m_map->GetWidth() * 2;
    int h = m_map->GetHeight() * 4;
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            const World::ResourceNode& node = m_map->GetResourceNode(x, y);
            if (!node.surveyed || node.type == World::ResourceType_None || node.amount <= 0)
                continue;

            float wx, wy;
            coords.NodeTileToWorld(x, y, wx, wy);

            RenderOverlayMarker marker;
            marker.markerType = OVERLAY_MARKER_DEPOSIT;
            marker.resourceType = static_cast<uint8_t>(node.type);
            marker.transform.worldX = wx;
            marker.transform.worldY = wy - 40.0f;
            marker.transform.depthLayer = static_cast<int>(0.97f * 65535.0f);
            out.push_back(marker);
        }
    }
}

void GeologistOverlayPresentationSystem::CollectWorkingIndicator(
    const FrameContext& frame,
    std::vector<RenderOverlayMarker>& out)
{
    if (frame.overlay.geologistState != OverlayFrameState::GEOLOGIST_WORKING)
        return;
    if (frame.overlay.geologistTileX < 0 || frame.overlay.geologistTileY < 0)
        return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    float wx, wy;
    coords.NodeTileToWorld(
        frame.overlay.geologistTileX,
        frame.overlay.geologistTileY, wx, wy);

    RenderOverlayMarker marker;
    marker.markerType = OVERLAY_MARKER_WORKING;
    marker.transform.worldX = wx;
    marker.transform.worldY = wy - 50.0f;
    marker.transform.depthLayer = static_cast<int>(0.97f * 65535.0f);
    out.push_back(marker);
}

} // namespace Scene
