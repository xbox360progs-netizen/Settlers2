#pragma once
#include <stdint.h>

namespace Scene {

// Screen-space position after world→screen projection.
// Filled by ProjectionSystem, consumed by renderers.
struct ScreenTransform {
    int screenX;
    int screenY;
    uint32_t depth;       // final draw-order depth (WORD-compatible)
};

} // namespace Scene
