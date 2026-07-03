#pragma once
#include "../Settlers2/SimulationCore/World/WorldModel.h"

namespace Scenarios {

    // Creates an empty world with minimal dimensions.
    // Future: load from file, or generate test-specific worlds with flags, roads, buildings.
    inline World::WorldModel CreateEmptyWorld(uint32_t width = 64, uint32_t height = 64)
    {
        World::WorldModel world;
        world.width = width;
        world.height = height;
        return world;
    }

} // namespace Scenarios
