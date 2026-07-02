#pragma once
#include <stdint.h>

namespace Scene {

// Pure visual identity for a building, flag, or construction site — no spatial data.
// The renderer resolves the sprite from kind + buildingType + depleted + state.
struct BuildingVisual {
    uint8_t kind;              // 0 = flag, 1 = building, 2 = construction site
    uint8_t buildingType;      // World::BuildingType (for kind=1,2)
    bool    depleted;          // true → show depleted mine sprite
    uint8_t fsmState;          // 0=Idle, 1=Producing, 2=OutputFull (BuildingFSM enum)
    bool    hasWorker;         // true if a worker is present at the building
    uint32_t color;            // ARGB tint (0xFFFFFFFF = opaque white)

    BuildingVisual()
        : kind(0), buildingType(0), depleted(false)
        , fsmState(0), hasWorker(false)
        , color(0xFFFFFFFF)
    {}
};

} // namespace Scene
