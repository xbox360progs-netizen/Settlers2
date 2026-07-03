#pragma once
#include <vector>
#include "RenderTownHallPanel.h"
#include "../Shared/RenderBuildingHighlight.h"

namespace World {
    class FlagManager;
}
namespace Logic {
    class EconomyManager;
}

namespace Scene {

class TownHallPresentationSystem {
public:
    void SetManagers(World::FlagManager* flagManager,
                     Logic::EconomyManager* economyManager);

    void BuildRenderFrame(const struct FrameContext& frame,
                          RenderTownHallPanel& panel,
                          std::vector<RenderBuildingHighlight>& highlights);

private:
    World::FlagManager*    m_flagManager;
    Logic::EconomyManager* m_economyManager;
};

} // namespace Scene
