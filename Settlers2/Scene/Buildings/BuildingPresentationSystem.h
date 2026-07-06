#pragma once
#include "../Shared/RenderFrame.h"
#include "../Presentation/Migration/IBuildingSource.h"
#include "../Presentation/Migration/IFlagSource.h"

// Produces RenderBuilding DTOs from IBuildingSource, IConstructionSiteSource,
// and IFlagSource. Called once per frame from GameScene::Update().
// Covers flags, completed buildings, and construction sites.

namespace Scene {

class BuildingPresentationSystem {
public:
    void SetSources(
        IFlagSource* flagSource,
        IBuildingSource* buildingSource,
        IConstructionSiteSource* constructionSiteSource
    );

    void BuildRenderFrame(RenderFrame& frame);

private:
    void CollectFlags(std::vector<RenderBuilding>& out);
    void CollectBuildings(std::vector<RenderBuilding>& out);
    void CollectConstructionSites(std::vector<RenderBuilding>& out);

    IFlagSource*            m_flagSource;
    IBuildingSource*        m_buildingSource;
    IConstructionSiteSource* m_constructionSiteSource;
};

} // namespace Scene
