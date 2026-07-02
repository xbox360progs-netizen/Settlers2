#include "stdafx.h"
#include "RoadPreviewPresentationSystem.h"
#include "../RoadController.h"
#include "../PlacementController.h"
#include "../../Logic/CoordinateSystem.h"

namespace Scene {

void RoadPreviewPresentationSystem::SetControllers(
    RoadController* roadController,
    PlacementController* placementController)
{
    m_roadController = roadController;
    m_placementController = placementController;
}

static void AddTileSegment(float wx, float wy, bool valid,
    std::vector<RenderRoadSegment>& out)
{
    RenderRoadSegment seg;
    seg.worldX0 = wx; seg.worldY0 = wy;
    seg.worldX1 = wx; seg.worldY1 = wy;
    seg.valid = valid;
    out.push_back(seg);
}

static void AddConnectionSegment(float wx0, float wy0, float wx1, float wy1, bool valid,
    std::vector<RenderRoadSegment>& out)
{
    RenderRoadSegment seg;
    seg.worldX0 = wx0; seg.worldY0 = wy0;
    seg.worldX1 = wx1; seg.worldY1 = wy1;
    seg.valid = valid;
    out.push_back(seg);
}

void RoadPreviewPresentationSystem::CollectPathTiles(
    std::vector<RenderRoadSegment>& out)
{
    CoordinateSystem& coords = CoordinateSystem::GetInstance();

    // Interactive path (white) — from user's manual road placement.
    const std::vector<std::pair<int,int> >& path = m_roadController->GetPreviewPath();
    if (!path.empty()) {
        for (size_t i = 0; i < path.size(); ++i) {
            int px = path[i].first;
            int py = path[i].second;
            float wx, wy;
            coords.NodeTileToWorld(px, py, wx, wy);
            AddTileSegment(wx, wy, true, out);

            // Horizontal connection quads between adjacent tiles.
            if (i + 1 < path.size()) {
                int nx = path[i + 1].first;
                int ny = path[i + 1].second;
                if (abs(px - nx) == 1 && py == ny) {
                    float nwx, nwy;
                    coords.NodeTileToWorld(nx, ny, nwx, nwy);
                    AddConnectionSegment(wx, wy, nwx, nwy, true, out);
                }
            }
        }
        return;
    }

    // Auto-path (blue in legacy, white in simplified model).
    const std::vector<std::pair<int,int> >& autoPath = m_roadController->GetAutoPath();
    if (!autoPath.empty()) {
        for (size_t i = 0; i < autoPath.size(); ++i) {
            int ax = autoPath[i].first;
            int ay = autoPath[i].second;
            float wx, wy;
            coords.NodeTileToWorld(ax, ay, wx, wy);
            AddTileSegment(wx, wy, true, out);

            if (i + 1 < autoPath.size()) {
                int nx = autoPath[i + 1].first;
                int ny = autoPath[i + 1].second;
                if (abs(ax - nx) == 1 && ay == ny) {
                    float nwx, nwy;
                    coords.NodeTileToWorld(nx, ny, nwx, nwy);
                    AddConnectionSegment(wx, wy, nwx, nwy, true, out);
                }
            }
        }
    }
}

void RoadPreviewPresentationSystem::CollectNeighborHints(
    std::vector<RenderRoadSegment>& out)
{
    const std::vector<std::pair<int,int> >& neighbors = m_roadController->GetValidNeighbors();
    if (neighbors.empty()) return;

    CoordinateSystem& coords = CoordinateSystem::GetInstance();
    for (size_t i = 0; i < neighbors.size(); ++i) {
        int nx = neighbors[i].first;
        int ny = neighbors[i].second;
        float wx, wy;
        coords.NodeTileToWorld(nx, ny, wx, wy);
        AddTileSegment(wx, wy, false, out);
    }
}

void RoadPreviewPresentationSystem::BuildRenderFrame(
    std::vector<RenderRoadSegment>& out)
{
    if (!m_roadController || !m_placementController) return;
    if (m_placementController->GetState() != PLACESTATE_PLACE_ROAD) return;

    CollectPathTiles(out);
    CollectNeighborHints(out);
}

} // namespace Scene
