#pragma once
#include "../Shared/RenderFrame.h"

namespace World {
    class FlagManager;
}

namespace Scene {

// Reads FlagManager to produce RenderBuilding DTOs from simulation state.
// Called once per frame from GameScene::Update().
// No rendering code, no sprite knowledge.
class BuildingPresentationSystem {
public:
    void SetManagers(
        World::FlagManager* flagManager
    );

    // Populates frame.buildings from simulation sources (flags).
    void BuildRenderFrame(RenderFrame& frame);

private:
    void CollectFlags(std::vector<RenderBuilding>& out);

    World::FlagManager* m_flagManager;
};

} // namespace Scene
