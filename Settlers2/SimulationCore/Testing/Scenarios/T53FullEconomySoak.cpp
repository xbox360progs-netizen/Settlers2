#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/EconomyMetrics.h"
#include "../../Testing/Assertions/SimulationAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Core/ProductionTypes.h"
#include "../../Core/TreeSystem.h"
#include "../../Systems/EconomySystem.h"
#include "../../Systems/WarehouseSystem.h"
#include <stdio.h>

namespace World {

    // Full Economy Soak — 500k ticks with all industries active.
    //
    // Pre-seeds every production building type with enough initial resources
    // to sustain production through the full run. SettlementSystem remains
    // enabled to make expansion decisions (more Foresters, additional buildings).
    //
    // Records EconomySnapshots at checkpoints (10k/50k/100k/250k/500k)
    // and produces an analytical report comparing trends across the run.
    //
    // Verifies:
    //   - All resource types produced continuously (no supply crises)
    //   - Flow <= Potential invariant
    //   - No tree/animal/fish depletion
    //   - Stable or growing resource flows
    //   - Demand backlog bounded

    static void SeedFullEconomy(WorldModel& world)
    {
        world.width = 50;
        world.height = 50;

        // Trees, animals, fish
        SeedTrees(world, 200, 500);
        world.animalCount = 50;
        world.maxAnimalCount = 50;
        world.fishCount = 50;
        world.maxFishCount = 50;

        int idx = 0;

        // Helper lambda for setting up production buildings
        // (using a struct and loop to avoid C++03 lambda limitation)

        // 0: Woodcutter
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Woodcutter;
            pb.position = Vector2i(10, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.outputResources[0] = ResourceType_Wood;
        }

        // 1: Forester
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Forester;
            pb.position = Vector2i(12, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
        }

        // 2: Sawmill (needs 2 Wood per cycle)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Sawmill;
            pb.position = Vector2i(14, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_Wood; pb.inputRequired[0] = 2;
            pb.inputDelivered[0] = 10; // pre-delivered for 5 cycles
            pb.outputResources[0] = ResourceType_Planks;
        }

        // 3: Stonemason
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Stonemason;
            pb.position = Vector2i(16, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.outputResources[0] = ResourceType_Stone;
        }

        // 4: Hunter
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Hunter;
            pb.position = Vector2i(18, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.outputResources[0] = ResourceType_Meat;
        }

        // 5: Fisher
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Fisher;
            pb.position = Vector2i(20, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.outputResources[0] = ResourceType_Fish;
        }

        // 6: CoalMine (needs food)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_CoalMine;
            pb.position = Vector2i(22, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.fed = true; pb.foodStored = 50;
            pb.outputResources[0] = ResourceType_Coal;
        }

        // 7: IronMine (needs food)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_IronMine;
            pb.position = Vector2i(24, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.fed = true; pb.foodStored = 50;
            pb.outputResources[0] = ResourceType_IronOre;
        }

        // 8: GoldMine (needs food)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_GoldMine;
            pb.position = Vector2i(26, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.fed = true; pb.foodStored = 50;
            pb.outputResources[0] = ResourceType_GoldOre;
        }

        // 9: IronSmelter (IronOre → IronBar)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_IronSmelter;
            pb.position = Vector2i(28, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_IronOre; pb.inputRequired[0] = 1;
            pb.inputDelivered[0] = 20;
            pb.outputResources[0] = ResourceType_IronBar;
        }

        // 10: Toolmaker (Wood + Stone → Tools)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Toolmaker;
            pb.position = Vector2i(30, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_Wood; pb.inputRequired[0] = 1;
            pb.inputDelivered[0] = 20;
            pb.inputResources[1] = ResourceType_Stone; pb.inputRequired[1] = 1;
            pb.inputDelivered[1] = 20;
            pb.outputResources[0] = ResourceType_Tools;
        }

        // 11: WeaponSmith (IronBar + Coal → Weapons)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_WeaponSmith;
            pb.position = Vector2i(32, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_IronBar; pb.inputRequired[0] = 1;
            pb.inputDelivered[0] = 20;
            pb.inputResources[1] = ResourceType_Coal; pb.inputRequired[1] = 1;
            pb.inputDelivered[1] = 20;
            pb.outputResources[0] = ResourceType_Weapons;
        }

        // 12: Farm (Wheat)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Farm;
            pb.position = Vector2i(34, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.outputResources[0] = ResourceType_Wheat;
        }

        // 13: Mill (Wheat → Flour)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Mill;
            pb.position = Vector2i(36, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_Wheat; pb.inputRequired[0] = 1;
            pb.inputDelivered[0] = 20;
            pb.outputResources[0] = ResourceType_Flour;
        }

        // 14: Bakery (Flour → Bread)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Bakery;
            pb.position = Vector2i(38, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_Flour; pb.inputRequired[0] = 1;
            pb.inputDelivered[0] = 20;
            pb.outputResources[0] = ResourceType_Bread;
        }

        // 15: Barracks (Weapons → Soldiers)
        {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Barracks;
            pb.position = Vector2i(40, 10); pb.owner = 0;
            pb.cycleTimer = 0; pb.active = true;
            pb.inputResources[0] = ResourceType_Weapons; pb.inputRequired[0] = 1;
            pb.inputDelivered[0] = 20;
            pb.outputResources[0] = ResourceType_Soldiers;
        }
    }

    // Checkpoint ticks for snapshot recording
    static const uint32_t kCheckpoints[] = {
        10000, 50000, 100000, 250000, 500000
    };
    static const int kCheckpointCount = sizeof(kCheckpoints) / sizeof(kCheckpoints[0]);

    class T53FullEconomySoak : public ISimulationScenario {
    public:
        T53FullEconomySoak()
            : m_checkpointIndex(0)
            , m_windowSampleCount(0)
        {
            for (int i = 0; i < EconomyFlowTracker::kMaxResources; ++i) {
                m_latestStats[i].sampleCount = 0;
            }
        }

        const char* GetName() const { return "T53"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableWarehouse = true;
            config.enableSettlement = true;
            config.enableTreeDepletion = true;
            config.enableConsumption = true;
        }

        void Initialize(Simulation& sim)
        {
            WorldModel world;
            SeedFullEconomy(world);
            sim.LoadWorld(world);
        }

        bool Tick(Simulation& sim)
        {
            uint32_t currentTick = sim.GetState().tickCount;
            const uint32_t kSoakTicks = 500000;
            const uint32_t kCheckInterval = 10000;

            if (currentTick > 0 && currentTick % kCheckInterval == 0) {
                if (!Assert::AllInvariants(sim.GetWorld())) {
                    printf("[FAIL][T53] Invariant failed at tick %u\n", currentTick);
                    return false;
                }
            }

            // Accumulate per-tick flow data for stability tracking
            EconomySystem* eco = sim.GetEconomySystem();
            if (eco != NULL) {
                m_tracker.AccumulateTick(eco);
                m_windowSampleCount++;
                // Flush window every 1000 ticks
                if (currentTick > 0 && m_windowSampleCount >= 1000) {
                    m_tracker.FlushWindow(m_latestStats);
                    m_windowSampleCount = 0;
                }
            }

            // Record snapshot at each checkpoint
            if (m_checkpointIndex < kCheckpointCount && currentTick == kCheckpoints[m_checkpointIndex]) {
                WarehouseSystem* wh = sim.GetWarehouseSystem();
                EconomySnapshot s = CollectSnapshot(sim.GetWorld(), eco, wh);
                s.tick = currentTick;
                m_history.snapshots[m_history.snapshotCount] = s;
                m_history.snapshotCount++;
                m_checkpointIndex++;

                // Print on-the-fly progress
                if (currentTick % 100000 == 0) {
                    printf("  [%6u] Checkpoint recorded (%d/%d)\n",
                        currentTick, m_checkpointIndex, kCheckpointCount);
                }
            }

            if (currentTick >= kSoakTicks) {
                return Verify(sim);
            }
            return true;
        }

        bool Verify(Simulation& sim)
        {
            const WorldModel& world = sim.GetWorld();
            EconomySystem* eco = sim.GetEconomySystem();
            if (eco == NULL) {
                printf("[FAIL][T53] EconomySystem not available\n");
                return false;
            }

            bool ok = true;

            // 1. Snapshot comparison report (trends, crises, ecology)
            if (!ReportSnapshotComparison(m_history, sim.GetState().tickCount, "T53")) {
                ok = false;
            }

            // 2. Flow <= Potential invariant (overall check)
            printf("\n  Flow vs potential check:\n");
            ResourceType flowResources[] = {
                ResourceType_Wood, ResourceType_Planks, ResourceType_Stone,
                ResourceType_Coal, ResourceType_IronOre, ResourceType_GoldOre,
                ResourceType_IronBar, ResourceType_Tools, ResourceType_Weapons,
                ResourceType_Meat, ResourceType_Fish, ResourceType_Wheat,
                ResourceType_Flour, ResourceType_Bread, ResourceType_Soldiers
            };
            int flowCount = sizeof(flowResources) / sizeof(flowResources[0]);
            bool allFlowValid = true;
            for (int r = 0; r < flowCount; ++r) {
                int flow = eco->GetResourceFlow(flowResources[r]);
                float potential = eco->GetProductionPotential(flowResources[r], world);
                if (flow > static_cast<int>(potential * EconomySystem::kFlowWindow + 0.5f)) {
                    printf("    Resource %d: flow=%d exceeds potential=%.4f*%d [FAIL]\n",
                        (int)flowResources[r], flow, potential, EconomySystem::kFlowWindow);
                    allFlowValid = false;
                    ok = false;
                }
            }
            if (allFlowValid) {
                printf("    [PASS] Flow <= potential for all resources\n");
            }

            // 3. Economy stability report (last complete window)
            if (m_windowSampleCount > 0) {
                m_tracker.FlushWindow(m_latestStats);
                m_windowSampleCount = 0;
            }
            int streaks[EconomyFlowTracker::kMaxResources];
            m_tracker.GetOscillationStreaks(streaks);

            printf("  Stability analysis (last window):\n");
            PrintStabilityReport(m_latestStats, EconomyFlowTracker::kMaxResources, "T53", streaks);
            PrintStabilityPropagation(m_latestStats, EconomyFlowTracker::kMaxResources, "T53", streaks);

            // 4. Final summary
            if (ok) {
                printf("\n[PASS][T53] Full economy stable for 500000 ticks\n");
            } else {
                printf("\n[FAIL][T53] Some invariants violated\n");
            }

            return false;
        }

    private:
        int m_checkpointIndex;
        EconomySnapshotHistory m_history;
        EconomyFlowTracker m_tracker;
        WindowFlowStats m_latestStats[EconomyFlowTracker::kMaxResources];
        int m_windowSampleCount;
    };

    T53FullEconomySoak g_t53;

} // namespace World
