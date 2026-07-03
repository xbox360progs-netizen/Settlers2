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

    // Gamepad cursor — separate from tile cursor, always shown when gamepad active
    bool   gamepadActive;
    int    gamepadTileX;
    int    gamepadTileY;
    float  gamepadWorldX;
    float  gamepadWorldY;
    int    gamepadScreenX;
    int    gamepadScreenY;

    RenderCursor()
        : worldX(0), worldY(0), screenX(0), screenY(0), valid(false)
        , gamepadActive(false), gamepadTileX(0), gamepadTileY(0)
        , gamepadWorldX(0), gamepadWorldY(0)
        , gamepadScreenX(0), gamepadScreenY(0) {}
};

} // namespace Scene
