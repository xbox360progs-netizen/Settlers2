#pragma once
#include <stdint.h>
#include "../../../Core/Vector2i.h"

// Temporary DTO used only during World → SimulationCore migration.
// Remove after LegacyCarrierSource is deleted.
//
// Carrier visualization reads position data from legacy Carrier/road structures.
// SimulationCore TransportCarrier has no spatial data yet — position comes from
// World::Carrier until Milestone 4 (position-computing model).

namespace Scene {

struct CarrierView {
    uint8_t state;          // World::CarrierState

    // Transit tiles (WalkingToPost/ReturningHome states)
    const Vector2i* transitTiles;
    uint32_t transitCount;
    float transitProgress;

    // Road tiles (Working state)
    const Vector2i* roadTiles;
    uint32_t roadTileCount;
    float roadEp;

    float walkDir;

    bool cargoPresent;
    uint8_t cargoType;      // ResourceType

    CarrierView()
        : state(0)
        , transitTiles(NULL), transitCount(0), transitProgress(0.0f)
        , roadTiles(NULL), roadTileCount(0), roadEp(0.0f)
        , walkDir(1.0f)
        , cargoPresent(false), cargoType(0)
    {
    }
};

} // namespace Scene
