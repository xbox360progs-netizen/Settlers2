#pragma once
#include <stdint.h>

namespace Scene {

// Per-frame cursor rendering DTO.
// Built by CursorPresentationSystem (world coords + valid flag),
// projected to screen coords by ProjectionSystem,
// consumed by CursorPass.
struct RenderCursor {
    float  worldX;
    float  worldY;
    int    screenX;
    int    screenY;
    bool   valid;

    RenderCursor() : worldX(0), worldY(0), screenX(0), screenY(0), valid(false) {}
};

} // namespace Scene
