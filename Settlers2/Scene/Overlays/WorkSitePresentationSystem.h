#pragma once
#include <vector>
#include "RenderWorkSite.h"

namespace Logic {
    class EconomyManager;
}

namespace Scene {

// Reads EconomyManager building list, queries GetWorkSiteRenderInfo for each,
// produces RenderWorkSite DTOs with pre-resolved sprite indices.
class WorkSitePresentationSystem {
public:
    void SetEconomyManager(Logic::EconomyManager* economy);
    void BuildRenderFrame(std::vector<RenderWorkSite>& out);

private:
    Logic::EconomyManager* m_economy;
};

} // namespace Scene
