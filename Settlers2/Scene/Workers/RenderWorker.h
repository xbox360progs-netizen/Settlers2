#pragma once
#include <stdint.h>
#include "../Shared/RenderTransform.h"

namespace Scene {

// Pure visual identity for any worker type (carrier, builder, building worker).
// Produced by BuildingWorkerPresentation + CarrierPresentation, consumed by WorkerPass.
// All spatial + visual state in one DTO — no simulation pointers.
struct RenderWorker {
    RenderTransform transform;

    uint8_t type;             // SettlerType enum (Carrier=0, Builder=1, Worker=2, BuildingWorker=3)
    uint8_t state;            // SettlerState (Walking=0, Idle=1, Working=2, Building=3)
    int8_t  dx;               // direction delta x (−1, 0, 1)
    int8_t  dy;               // direction delta y (−1, 0, 1)
    uint8_t carrying : 1;     // 1 if carrying cargo
    uint8_t cargoType : 7;    // ResourceType enum
    uint8_t buildingType;     // BuildingType for profession-specific sprites (255 = none)
    uint8_t animationFrame;   // future: animation frame index (reserved, 0 for now)
};

} // namespace Scene
