#pragma once
#include <vector>
#include "../FrameContext.h"
#include "RenderOverlayMarker.h"

namespace World { class Map; }

namespace Scene {

// Builds RenderOverlayMarker DTOs from game state:
//   1. Mountain highlight — cursor over mountain/rock tile
//   2. Surveyed deposit icons — resource nodes marked as surveyed
//   3. Geologist working indicator — active exploration site
class GeologistOverlayPresentationSystem {
public:
    void SetMap(World::Map* map) { m_map = map; }

    void BuildRenderFrame(const FrameContext& frame,
                          std::vector<RenderOverlayMarker>& out);

private:
    World::Map* m_map;

    void CollectMountainHighlight(const FrameContext& frame,
                                  std::vector<RenderOverlayMarker>& out);
    void CollectSurveyedDeposits(std::vector<RenderOverlayMarker>& out);
    void CollectWorkingIndicator(const FrameContext& frame,
                                 std::vector<RenderOverlayMarker>& out);
};

} // namespace Scene
