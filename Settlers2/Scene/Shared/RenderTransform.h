#pragma once
#include <stdint.h>

namespace Scene {

// Spatial identity for any renderable object.
// Pre-computed by PresentationSystem, consumed by renderers.
// worldX/worldY set by Presentation; screenX/screenY set by ProjectionSystem.
struct RenderTransform {
    float worldX;
    float worldY;
    int   depthLayer;          // pre-computed draw order (e.g. 30010 + tileY*400)

    int   screenX;             // pre-computed screen X (set by ProjectionSystem)
    int   screenY;             // pre-computed screen Y (set by ProjectionSystem)

    RenderTransform() : worldX(0), worldY(0), depthLayer(0), screenX(0), screenY(0) {}
};

} // namespace Scene
