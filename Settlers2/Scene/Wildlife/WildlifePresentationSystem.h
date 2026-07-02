#pragma once
#include <vector>
#include "RenderWildlife.h"

namespace World {
    class WildlifeSystem;
    class AnimalManager;
}

namespace Scene {

// Reads all alive animals from WildlifeSystem and produces RenderWildlife
// DTOs with world coords and visual identity (type, state, direction).
// Called once per frame from GameScene::Update().
// No rendering code, no sprite knowledge.
class WildlifePresentationSystem {
public:
    void SetWildlifeSystem(World::WildlifeSystem* wildlife);

    // Populates out list from all alive animals.
    void BuildRenderFrame(std::vector<RenderWildlife>& out);

private:
    World::WildlifeSystem* m_wildlife;
};

} // namespace Scene
