#pragma once
#include <vector>
#include "../Shared/RenderFrame.h"

namespace World {
    class CarrierManager;
    class ConstructionManager;
    class WorkerManager;
    class FlagManager;
    class RoadManager;
}

namespace Scene {

// Unified presentation system for all worker types.
// Reads from all simulation worker sources and produces RenderWorker DTOs.
// Single source of truth for worker rendering — replaces SettlerPresentationSystem worker parts.
class WorkerPresentationSystem {
public:
    WorkerPresentationSystem();

    void SetManagers(
        World::CarrierManager* carrierManager,
        World::ConstructionManager* constructionManager,
        World::WorkerManager* workerManager,
        World::FlagManager* flagManager,
        World::RoadManager* roadManager);

    void BuildRenderFrame(RenderFrame& frame);

private:
    void CollectCarriers(std::vector<RenderWorker>& out);
    void CollectBuilders(std::vector<RenderWorker>& out);
    void CollectMovingWorkers(std::vector<RenderWorker>& out);
    void CollectBuildingWorkers(std::vector<RenderWorker>& out);

    World::CarrierManager*      m_carrierManager;
    World::ConstructionManager* m_constructionManager;
    World::WorkerManager*       m_workerManager;
    World::FlagManager*         m_flagManager;
    World::RoadManager*         m_roadManager;
};

} // namespace Scene
