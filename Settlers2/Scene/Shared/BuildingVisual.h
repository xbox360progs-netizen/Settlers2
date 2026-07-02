#pragma once
#include <stdint.h>

namespace Scene {

// Pure visual identity for a building or flag — no spatial data.
// The renderer resolves the sprite from kind + buildingType + depleted.
struct BuildingVisual {
    uint8_t kind;              // 0 = flag, 1 = building
    uint8_t buildingType;      // World::BuildingType (for kind=1)
    bool    depleted;          // true → show depleted mine sprite
    uint32_t color;            // ARGB tint (0xFFFFFFFF = opaque white)
};

} // namespace Scene
