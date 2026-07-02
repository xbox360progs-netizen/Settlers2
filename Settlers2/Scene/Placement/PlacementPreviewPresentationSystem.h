#pragma once
#include "RenderPlacementPreview.h"
#include "../FrameContext.h"

namespace Scene {

class PlacementController;

// Reads PlacementController state and produces RenderPlacementPreview DTOs
// with world coords and pre-computed validity.
// Called once per frame from GameScene::Update().
// No rendering code, no sprite knowledge.
class PlacementPreviewPresentationSystem {
public:
    void SetPlacementController(PlacementController* ctrl) { m_placement = ctrl; }

    // Populates out with a single preview DTO if placement mode is active.
    void BuildRenderFrame(const FrameContext& frame, std::vector<RenderPlacementPreview>& out);

private:
    PlacementController* m_placement;
};

} // namespace Scene
