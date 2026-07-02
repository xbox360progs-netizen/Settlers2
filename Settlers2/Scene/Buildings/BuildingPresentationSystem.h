#pragma once
#include "../Shared/RenderFrame.h"

namespace World {
    class FlagManager;
    class ConstructionManager;
}

namespace Scene {

// Reads FlagManager + ConstructionManager to produce RenderBuilding DTOs.
// Called once per frame from GameScene::Update().
// Covers flags, completed buildings, and construction sites.
class BuildingPresentationSystem {
public:
    void SetManagers(
        World::FlagManager* flagManager,
        World::ConstructionManager* constructionManager
    );

    void BuildRenderFrame(RenderFrame& frame);

private:
    void CollectFlags(std::vector<RenderBuilding>& out);
    void CollectBuildings(std::vector<RenderBuilding>& out);
    void CollectConstructionSites(std::vector<RenderBuilding>& out);

    World::FlagManager*         m_flagManager;
    World::ConstructionManager* m_constructionManager;
};

} // namespace Scene
