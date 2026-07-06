#pragma once
#include <vector>
#include "../Shared/RenderFrame.h"

namespace Scene {

class IBuildingSource;
class IConstructionSiteSource;

// Renders workers attached to buildings: production workers + builders.
// Reads from IBuildingSource (completed buildings) and IConstructionSiteSource
// (construction sites). Produces RenderWorker DTOs for BuildingWorker (type=3)
// and Builder (type=1). Position is the building/flag location — no road path,
// no CarrierManager dependency.
class BuildingWorkerPresentation {
public:
    BuildingWorkerPresentation();

    void SetSources(
        IBuildingSource* buildingSource,
        IConstructionSiteSource* constructionSiteSource);

    void BuildRenderFrame(RenderFrame& frame);

private:
    void CollectBuildingWorkers(std::vector<RenderWorker>& out);
    void CollectBuilders(std::vector<RenderWorker>& out);

    IBuildingSource*         m_buildingSource;
    IConstructionSiteSource* m_constructionSiteSource;
};

} // namespace Scene
