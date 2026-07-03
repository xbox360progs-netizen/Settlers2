#pragma once
#include "../Shared/RenderTransform.h"

namespace Scene {

// Per-building work-site sprite DTO (e.g., mine framework at resource node).
struct RenderWorkSite {
    RenderTransform transform;
    int             spriteIdx;   // pre-resolved atlas index into Buildings atlas

    RenderWorkSite() : spriteIdx(-1) {}
};

} // namespace Scene
