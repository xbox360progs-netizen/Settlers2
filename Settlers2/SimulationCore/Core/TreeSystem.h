#pragma once
#include "../World/WorldModel.h"

namespace World {

    // Seed initial trees on the map. Total positions = matureCount + emptySpots.
    inline void SeedTrees(WorldModel& world, int matureCount, int emptySpots)
    {
        world.treeMatureCount = matureCount;
        world.treeYoungCount = 0;
        world.treeSaplingCount = 0;
        world.treeStumpCount = 0;
        world.treeEmptySpots = emptySpots;
    }

    // Seed initial animals on the map.
    inline void SeedAnimals(WorldModel& world, int count, int maxCount)
    {
        world.animalCount = count;
        world.maxAnimalCount = maxCount;
    }

    // Seed initial fish on the map.
    inline void SeedFish(WorldModel& world, int count, int maxCount)
    {
        world.fishCount = count;
        world.maxFishCount = maxCount;
    }

}
