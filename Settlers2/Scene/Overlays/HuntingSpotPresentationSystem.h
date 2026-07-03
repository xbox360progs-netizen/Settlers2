#pragma once
#include <vector>
#include "RenderOverlayMarker.h"

namespace World {
    class Map;
    class FlagManager;
}
namespace Scene {
    class PlacementController;
}

namespace Scene {

class HuntingSpotPresentationSystem {
public:
    void SetManagers(World::FlagManager* flagManager, World::Map* map,
                     PlacementController* placement);

    void BuildRenderFrame(const struct FrameContext& frame,
                          std::vector<RenderOverlayMarker>& out);

private:
    World::FlagManager*    m_flagManager;
    World::Map*            m_map;
    PlacementController*   m_placement;
};

} // namespace Scene
