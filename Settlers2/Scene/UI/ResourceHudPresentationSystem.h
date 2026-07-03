#pragma once
#include "RenderResourceHud.h"

namespace Logic {
    class EconomyManager;
}

namespace Scene {

struct FrameContext;

// Reads FrameContext.overlay.resourceHud + EconomyManager and produces
// RenderResourceHud DTO with pre-resolved icon UVs and stock counts.
class ResourceHudPresentationSystem {
public:
    void SetEconomyManager(Logic::EconomyManager* economy);
    void BuildRenderFrame(const FrameContext& frame, RenderResourceHud& out);

private:
    Logic::EconomyManager* m_economy;
};

} // namespace Scene
