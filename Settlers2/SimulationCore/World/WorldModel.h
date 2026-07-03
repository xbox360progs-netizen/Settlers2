#pragma once
#include <stdint.h>

namespace World {

    // Domain data for the simulation world.
    // Currently a skeleton — populated as subsystems are migrated into SimulationCore.
    // This is PURE DATA, not managers or systems.
    struct WorldModel {
        uint32_t width;
        uint32_t height;

        // Future:
        //   Tile[] tiles;
        //   Flag[] flags;
        //   Road[] roads;
        //   Building[] buildings;
        //   Worker[] workers;

        WorldModel()
            : width(0)
            , height(0)
        {
        }
    };

} // namespace World
