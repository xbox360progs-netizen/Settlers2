#pragma once
#include <vector>
#include <stdint.h>

namespace World {
    class FlagManager;
    class CarrierManager;
}

namespace Scene {

struct RenderDebugLabel;

class LogisticsDebugPresentationSystem {
public:
    void SetManagers(World::FlagManager* flagManager,
                     World::CarrierManager* carrierManager);

    void BuildRenderFrame(const struct FrameContext& frame,
                          std::vector<RenderDebugLabel>& labels);

private:
    World::FlagManager*    m_flagManager;
    World::CarrierManager* m_carrierManager;
};

} // namespace Scene
