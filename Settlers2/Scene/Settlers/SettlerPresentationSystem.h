#pragma once
#include "RenderSettler.h"
#include "../Shared/RenderFrame.h"
#include <vector>

namespace World {
    class CarrierManager;
    class ConstructionManager;
    class WorkerManager;
    class RoadManager;
}

namespace Scene {

// Builds RenderFrame from simulation state.
// Called once per frame from GameScene::Update().
// No rendering code, no sprite knowledge.
class SettlerPresentationSystem {
public:
    void SetManagers(
        World::CarrierManager* carrierManager,
        World::ConstructionManager* constructionManager,
        World::WorkerManager* workerManager,
        World::RoadManager* roadManager
    );

    // Populates frame.settlers from all simulation sources (carriers, builders, workers).
    void BuildRenderFrame(RenderFrame& frame);

private:
    World::CarrierManager*       m_carrierManager;
    World::ConstructionManager*  m_constructionManager;
    World::WorkerManager*        m_workerManager;
    World::RoadManager*          m_roadManager;
};

} // namespace Scene
