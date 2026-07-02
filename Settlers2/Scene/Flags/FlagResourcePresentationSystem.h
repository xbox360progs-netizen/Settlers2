#pragma once
#include <vector>
#include "RenderFlagResource.h"

namespace World {
    class FlagManager;
}

namespace Scene {

// Reads flag inventory from FlagManager and produces RenderFlagResource DTOs
// with world coords. ProjectionSystem later transforms to screen coords.
class FlagResourcePresentationSystem {
public:
    void SetFlagManager(World::FlagManager* mgr) { m_flagManager = mgr; }

    void BuildRenderFrame(std::vector<RenderFlagResource>& outResources);

private:
    World::FlagManager* m_flagManager;
};

} // namespace Scene
