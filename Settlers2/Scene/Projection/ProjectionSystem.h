#pragma once
#include "../Shared/RenderFrame.h"

class Camera;

namespace Scene {

// Converts world coordinates in a RenderFrame to screen coordinates.
// Called once per frame after all Presentation systems have run.
// The renderers then read screenX/screenY instead of worldX/worldY.
// Now handles all renderable categories: buildings, terrain, cursor, flag resources, wildlife, preview, road, work sites, road connections, workers, overlays.
class ProjectionSystem {
public:
    void SetCamera(Camera* camera);

    // Transforms all world coords in frame to screen coords.
    void Project(RenderFrame& frame);

private:
    void ProjectBuildings(RenderFrame& frame);
    void ProjectTerrain(RenderFrame& frame);
    void ProjectCursor(RenderFrame& frame);
    void ProjectFlagResources(RenderFrame& frame);
    void ProjectWildlife(RenderFrame& frame);
    void ProjectPlacementPreview(RenderFrame& frame);
    void ProjectRoadPreview(RenderFrame& frame);
    void ProjectOverlays(RenderFrame& frame);
    void ProjectGroundResources(RenderFrame& frame);
    void ProjectWorkers(RenderFrame& frame);
    void ProjectHighlights(RenderFrame& frame);
    void ProjectRoadConnections(RenderFrame& frame);
    void ProjectWorkSites(RenderFrame& frame);

    Camera* m_camera;
};

} // namespace Scene
