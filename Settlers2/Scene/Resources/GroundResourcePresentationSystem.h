#pragma once
#include <vector>
#include "../Shared/RenderFrame.h"

namespace World { class Map; }

namespace Scene {

class GroundResourcePresentationSystem {
public:
    GroundResourcePresentationSystem();

    void SetMap(World::Map* map) { m_map = map; }

    void BuildRenderFrame(RenderFrame& frame);

private:
    World::Map* m_map;
};

} // namespace Scene
