#pragma once
#include <vector>
#include "RenderRoadConnection.h"

namespace World {
    class Map;
}

namespace Scene {

// Reads committed road tiles from Map layer and produces RenderRoadConnection DTOs.
// Called once per frame from GameScene::Update().
class RoadConnectionPresentationSystem {
public:
    void SetMap(World::Map* map);
    void BuildRenderFrame(std::vector<RenderRoadConnection>& out);

private:
    World::Map* m_map;
};

} // namespace Scene
