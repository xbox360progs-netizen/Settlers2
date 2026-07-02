#pragma once
#include <stdint.h>

namespace Scene {

// Pure visual identity for a wild animal — no spatial data.
// Only the renderer reads these fields for sprite resolution.
// The enum values are chosen to fit in uint8 to keep the DTO compact.
struct WildlifeVisual {
    uint8_t type;        // World::AnimalType (AnimalType_Deer=0, Rabbit=1, Crocodile=2, Snake=3)
    uint8_t state;       // World::AnimalState (AnimalState_Alive=0, AnimalState_Trapped=1)
    uint8_t dirIndex;    // Direction index 0-3 (NE, SE, NW, SW) pre-computed from velocity
};

} // namespace Scene
