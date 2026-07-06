#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"
#include "../Core/BuildingTypes.h"

namespace World {

    struct WorldModel;

    class RenewableResourceSystem : public ISimulationSystem {
    public:
        RenewableResourceSystem();
        ~RenewableResourceSystem();

        virtual void Tick(WorldModel& world);

        void SetEnabled(bool enabled) { m_enabled = enabled; }

        // World seeding — replaces TreeSystem.h functions
        void SeedTrees(WorldModel& world, int matureCount, int emptySpots);
        void SeedAnimals(WorldModel& world, int count, int maxCount);
        void SeedFish(WorldModel& world, int count, int maxCount);

        // Production queries — used by ProductionSystem
        // Handles renewable resource lifecycle for a production cycle completion.
        // Returns false if production should be skipped (resource not available).
        bool OnProductionCycle(BuildingType type, WorldModel& world);

    private:
        bool CanProduce(BuildingType type, const WorldModel& world);
        void Consume(BuildingType type, WorldModel& world);
        void PlantSapling(WorldModel& world);
        void AdvanceTreeGrowth(WorldModel& world);
        void RegenerateAnimals(WorldModel& world);
        void RegenerateFish(WorldModel& world);

        // Every growth cycle (100 ticks), natural regrowth produces
        // emptySpots / kNaturalRegrowthDivisor new saplings (minimum 1 if any empty spots).
        // Higher divisor = slower natural regrowth.
        static const int kNaturalRegrowthDivisor = 30;

        uint32_t m_tickCount;
        bool m_enabled;
    };

}
