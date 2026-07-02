#pragma once
#include <stdint.h>

namespace Scene {

// Pure visual identity for a settler — no spatial data.
// Only the renderer reads these fields for sprite resolution.
// The enum values are chosen to fit in uint8 to keep the DTO compact.
struct SettlerVisual {
    uint8_t type;             // SettlerType (SettlerType_Carrier=0, Builder=1, Worker=2, BuildingWorker=3)
    uint8_t state;            // SettlerState (Walking=0, Idle=1, Working=2, Building=3)
    int8_t  dx;               // direction delta x (−1, 0, 1)
    int8_t  dy;               // direction delta y (−1, 0, 1)
    uint8_t carrying : 1;     // 1 if carrying cargo
    uint8_t cargoType : 7;    // ResourceType enum
    uint8_t buildingType;     // BuildingType for profession-specific sprites (255 = none)
};

} // namespace Scene
