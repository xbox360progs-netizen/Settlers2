#pragma once
#include <stdint.h>
#include "RenderTransform.h"

namespace Scene {

// World-space building highlight (semi-transparent overlay).
// Produced by TownHallPresentationSystem from FlagManager.
// The Pass resolves the building sprite from BuildingType via atlas lookup.
// buildingType stores World::BuildingType as int (cross-layer DTO — no World includes).
struct RenderBuildingHighlight {
    RenderTransform  transform;
    int              buildingType;       // World::BuildingType enum value
    bool             isDepleted;         // use depleted sprite if true
    uint8_t          padding[3];         // alignment

    RenderBuildingHighlight()
        : buildingType(0)
        , isDepleted(false)
    {
        padding[0] = padding[1] = padding[2] = 0;
    }
};

} // namespace Scene
