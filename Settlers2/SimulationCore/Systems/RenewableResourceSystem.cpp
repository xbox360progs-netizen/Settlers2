#include "RenewableResourceSystem.h"
#include <string.h>
#include "../World/WorldModel.h"
#include "../Definitions/RenewableResourceDefinition.h"

namespace World {

    RenewableResourceSystem::RenewableResourceSystem()
        : m_tickCount(0)
        , m_enabled(false)
    {
    }

    RenewableResourceSystem::~RenewableResourceSystem()
    {
    }

    // ---- World seeding ----

    void RenewableResourceSystem::SeedTrees(WorldModel& world, int matureCount, int emptySpots)
    {
        world.treeMatureCount = matureCount;
        world.treeYoungCount = 0;
        world.treeSaplingCount = 0;
        world.treeStumpCount = 0;
        world.treeEmptySpots = emptySpots;
    }

    void RenewableResourceSystem::SeedAnimals(WorldModel& world, int count, int maxCount)
    {
        world.animalCount = count;
        world.maxAnimalCount = maxCount;
    }

    void RenewableResourceSystem::SeedFish(WorldModel& world, int count, int maxCount)
    {
        world.fishCount = count;
        world.maxFishCount = maxCount;
    }

    // ---- Tick ----

    void RenewableResourceSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        if (!m_enabled) return;

        if ((m_tickCount % 100) == 0) {
            AdvanceTreeGrowth(world);
            RegenerateAnimals(world);
            RegenerateFish(world);
        }
    }

    // ---- Production queries ----

    bool RenewableResourceSystem::OnProductionCycle(BuildingType type, WorldModel& world)
    {
        if (!m_enabled) return true;

        const RenewableResourceDefinition& def = GetRenewableResourceDefinition(type);
        if (def.buildingType != type) return true; // not a renewable producer

        // Producers with no harvested resource (e.g., Forester) plant instead of consume
        if (def.harvestedResource == ResourceType_None) {
            if (def.model == PM_StagedTrees) {
                PlantSapling(world);
            }
            return true;
        }

        if (!CanProduce(type, world)) return false;
        Consume(type, world);
        return true;
    }

    bool RenewableResourceSystem::CanProduce(BuildingType type, const WorldModel& world)
    {
        if (!m_enabled) return true;

        const RenewableResourceDefinition& def = GetRenewableResourceDefinition(type);
        if (def.buildingType != type) return true; // not a renewable producer

        switch (def.model) {
            case PM_StagedTrees:
                return world.treeMatureCount > 0;

            case PM_SimplePopulation:
                if (def.harvestedResource == ResourceType_Meat)
                    return world.animalCount > 0;
                if (def.harvestedResource == ResourceType_Fish)
                    return world.fishCount > 0;
                return true;
        }

        return true;
    }

    void RenewableResourceSystem::Consume(BuildingType type, WorldModel& world)
    {
        if (!m_enabled) return;

        const RenewableResourceDefinition& def = GetRenewableResourceDefinition(type);
        if (def.buildingType != type) return;

        switch (def.model) {
            case PM_StagedTrees:
                if (world.treeMatureCount > 0) {
                    world.treeMatureCount--;
                    world.treeStumpCount++;
                }
                break;

            case PM_SimplePopulation:
                if (def.harvestedResource == ResourceType_Meat && world.animalCount > 0) {
                    world.animalCount--;
                } else if (def.harvestedResource == ResourceType_Fish && world.fishCount > 0) {
                    world.fishCount--;
                }
                break;
        }
    }

    void RenewableResourceSystem::PlantSapling(WorldModel& world)
    {
        if (!m_enabled) return;
        if (world.treeEmptySpots <= 0) return;
        world.treeEmptySpots--;
        world.treeSaplingCount++;
    }

    // ---- Regeneration ----

    void RenewableResourceSystem::AdvanceTreeGrowth(WorldModel& world)
    {
        // Stumps decay to empty spots
        int decayedStumps = world.treeStumpCount;
        world.treeStumpCount = 0;
        world.treeEmptySpots += decayedStumps;

        // Natural regrowth: empty spots slowly produce saplings without Forester.
        // This prevents total forest extinction and enables bootstrap scenarios
        // where Forester may not be built yet.
        int naturalSaplings = world.treeEmptySpots / kNaturalRegrowthDivisor;
        if (naturalSaplings == 0 && world.treeEmptySpots > 0) {
            naturalSaplings = 1;  // minimum trickle when any space exists
        }
        if (naturalSaplings > 0) {
            if (naturalSaplings > world.treeEmptySpots)
                naturalSaplings = world.treeEmptySpots;
            world.treeEmptySpots -= naturalSaplings;
            world.treeSaplingCount += naturalSaplings;
        }

        // Young trees mature
        int matured = world.treeYoungCount;
        world.treeYoungCount = 0;
        world.treeMatureCount += matured;

        // Saplings grow to young (includes both Forester-planted and natural)
        int grew = world.treeSaplingCount;
        world.treeSaplingCount = 0;
        world.treeYoungCount += grew;
    }

    void RenewableResourceSystem::RegenerateAnimals(WorldModel& world)
    {
        const RenewableResourceDefinition& def = GetRenewableResourceDefinition(BuildingType_Hunter);
        if (def.buildingType != BuildingType_Hunter) return;

        int cap = static_cast<int>(def.capacity);
        if (world.animalCount < cap) {
            world.animalCount++;
            if (world.animalCount > cap)
                world.animalCount = cap;
        }
    }

    void RenewableResourceSystem::RegenerateFish(WorldModel& world)
    {
        const RenewableResourceDefinition& def = GetRenewableResourceDefinition(BuildingType_Fisher);
        if (def.buildingType != BuildingType_Fisher) return;

        int cap = static_cast<int>(def.capacity);
        if (world.fishCount < cap) {
            world.fishCount++;
            if (world.fishCount > cap)
                world.fishCount = cap;
        }
    }

}
