#include "stdafx.h"
#include "ProjectionSystem.h"
#include "../Terrain/RenderTerrainTile.h"
#include "../Cursor/RenderCursor.h"
#include "../Flags/RenderFlagResource.h"
#include "../Wildlife/RenderWildlife.h"
#include "../Placement/RenderPlacementPreview.h"
#include "../Roads/RenderRoadPreview.h"
#include "../Overlays/RenderOverlayMarker.h"
#include "../../Graphics/Camera.h"

namespace Scene {

void ProjectionSystem::SetCamera(Camera* camera)
{
    m_camera = camera;
}

void ProjectionSystem::Project(RenderFrame& frame)
{
    if (!m_camera) return;
    ProjectSettlers(frame);
    ProjectBuildings(frame);
    ProjectTerrain(frame);
    ProjectCursor(frame);
    ProjectFlagResources(frame);
    ProjectWildlife(frame);
    ProjectPlacementPreview(frame);
    ProjectRoadPreview(frame);
    ProjectOverlays(frame);
    ProjectGroundResources(frame);
    ProjectWorkers(frame);
}

void ProjectionSystem::ProjectSettlers(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.settlers.size(); ++i) {
        RenderTransform& t = frame.settlers[i].transform;
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = (int)(sx + 0.5f);
        t.screenY = (int)(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectBuildings(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.buildings.size(); ++i) {
        RenderTransform& t = frame.buildings[i].transform;
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = (int)(sx + 0.5f);
        t.screenY = (int)(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectTerrain(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.terrain.size(); ++i) {
        RenderTerrainTile& t = frame.terrain[i];
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = static_cast<float>(static_cast<int>(sx + 0.5f));
        t.screenY = static_cast<float>(static_cast<int>(sy + 0.5f));
    }
}

void ProjectionSystem::ProjectFlagResources(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.flagResources.size(); ++i) {
        RenderFlagResource& r = frame.flagResources[i];
        float sx, sy;
        m_camera->WorldToScreen(r.worldX, r.worldY, sx, sy);
        r.screenX = static_cast<int>(sx + 0.5f);
        r.screenY = static_cast<int>(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectWildlife(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.wildlife.size(); ++i) {
        RenderTransform& t = frame.wildlife[i].transform;
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = static_cast<int>(sx + 0.5f);
        t.screenY = static_cast<int>(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectPlacementPreview(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.preview.size(); ++i) {
        RenderTransform& t = frame.preview[i].transform;
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = static_cast<int>(sx + 0.5f);
        t.screenY = static_cast<int>(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectRoadPreview(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.roadPreview.size(); ++i) {
        RenderRoadSegment& seg = frame.roadPreview[i];
        float sx0, sy0, sx1, sy1;
        m_camera->WorldToScreen(seg.worldX0, seg.worldY0, sx0, sy0);
        m_camera->WorldToScreen(seg.worldX1, seg.worldY1, sx1, sy1);
        seg.screenX0 = static_cast<int>(sx0 + 0.5f);
        seg.screenY0 = static_cast<int>(sy0 + 0.5f);
        seg.screenX1 = static_cast<int>(sx1 + 0.5f);
        seg.screenY1 = static_cast<int>(sy1 + 0.5f);
    }
}

void ProjectionSystem::ProjectOverlays(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.overlays.size(); ++i) {
        RenderOverlayMarker& m = frame.overlays[i];
        float sx, sy;
        m_camera->WorldToScreen(m.transform.worldX, m.transform.worldY, sx, sy);
        m.transform.screenX = static_cast<int>(sx + 0.5f);
        m.transform.screenY = static_cast<int>(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectCursor(RenderFrame& frame)
{
    if (!frame.cursor.valid) return;
    float sx, sy;
    m_camera->WorldToScreen(
        frame.cursor.worldX, frame.cursor.worldY, sx, sy);
    frame.cursor.screenX = static_cast<int>(sx + 0.5f);
    frame.cursor.screenY = static_cast<int>(sy + 0.5f);
}

void ProjectionSystem::ProjectGroundResources(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.groundResources.size(); ++i) {
        RenderTransform& t = frame.groundResources[i].transform;
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = static_cast<int>(sx + 0.5f);
        t.screenY = static_cast<int>(sy + 0.5f);
    }
}

void ProjectionSystem::ProjectWorkers(RenderFrame& frame)
{
    for (size_t i = 0; i < frame.workers.size(); ++i) {
        RenderTransform& t = frame.workers[i].transform;
        float sx, sy;
        m_camera->WorldToScreen(t.worldX, t.worldY, sx, sy);
        t.screenX = static_cast<int>(sx + 0.5f);
        t.screenY = static_cast<int>(sy + 0.5f);
    }
}

} // namespace Scene
