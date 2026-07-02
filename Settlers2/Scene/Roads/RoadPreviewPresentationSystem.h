#pragma once
#include <vector>
#include "RenderRoadPreview.h"

namespace Scene {

class RoadController;
class PlacementController;

// Reads RoadController state (preview path, auto-path, valid neighbors)
// and produces RenderRoadSegment DTOs with world coords.
// Called once per frame from GameScene::Update().
// No rendering code, no sprite knowledge.
class RoadPreviewPresentationSystem {
public:
    void SetControllers(
        RoadController* roadController,
        PlacementController* placementController
    );

    // Populates out with road preview segments.
    void BuildRenderFrame(std::vector<RenderRoadSegment>& out);

private:
    RoadController*     m_roadController;
    PlacementController* m_placementController;

    void CollectPathTiles(std::vector<RenderRoadSegment>& out);
    void CollectNeighborHints(std::vector<RenderRoadSegment>& out);
};

} // namespace Scene
