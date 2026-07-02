#include "stdafx.h"
#include "SettlerPresentationSystem.h"
#include "../../World/CarrierManager.h"
#include "../../World/ConstructionManager.h"
#include "../../World/WorkerManager.h"
#include "../../World/RoadManager.h"

namespace Scene {

void SettlerPresentationSystem::SetManagers(
    World::CarrierManager* carrierManager,
    World::ConstructionManager* constructionManager,
    World::WorkerManager* workerManager,
    World::RoadManager* roadManager)
{
    m_carrierManager = carrierManager;
    m_constructionManager = constructionManager;
    m_workerManager = workerManager;
    m_roadManager = roadManager;
}

void SettlerPresentationSystem::BuildRenderFrame(RenderFrame& frame)
{
    // DEPRECATED: WorkerPresentationSystem now produces all worker DTOs.
    // Kept only to clear the vector until SettlerPass + SettlerRenderer are removed.
    frame.settlers.clear();
}

} // namespace Scene
