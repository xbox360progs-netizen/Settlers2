#pragma once
#include <stdint.h>

namespace Scene {

// Pure visual identity for placement preview — no spatial data.
// Only the renderer reads these fields for sprite resolution.
struct PlacementPreviewVisual {
    uint8_t type;        // World::BuildingType (Building_None=0 = no preview)
    bool    allowed;     // pre-computed validity (green vs red tint)
};

} // namespace Scene
