#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/EconomyMetrics.h"
#include "../../Testing/SoakHarness.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Systems/EconomySystem.h"
#include "../../Systems/RenewableResourceSystem.h"
#include "../../Core/TreeSystem.h"
#include <stdio.h>

namespace World {

    class T56RenewableEconomySoak : public ISimulationScenario {
    public:
        T56RenewableEconomySoak() : m_minTotalTrees(0) {}

        const char* GetName() const { return "T56"; }
        const char* GetDescription() const { return "RenewableEconomySoak — infinite Wood cycle at 50k ticks"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableWorkers = true;
            config.enableSettlement = true;
            config.enableTreeDepletion = true;
            config.enableConsumption = false;
            config.enableWarehouse = false;
        }

        void Initialize(Simulation& sim)
        {
            WorldModel world;
            world.width = 50;
            world.height = 50;

            // Seed balanced forest: enough mature trees for initial production,
            // enough empty spots for natural regrowth and Forester planting.
            SeedTrees(world, 100, 400);

            sim.LoadWorld(world);

            // Add workers to execute construction jobs
            WorldModel& loaded = sim.GetWorld();
            for (int i = 0; i < 10; ++i) {
                if (loaded.workerCount >= kMaxWorkers) break;
                Worker& w = loaded.workers[loaded.workerCount++];
                w.id = i;
                w.state = WorkerState_Idle;
                w.currentJob = 0;
                w.workTicksRemaining = 0;
            }

            m_minTotalTrees = 1000000; // will be set on first tick
        }

        bool Tick(Simulation& sim)
        {
            const WorldModel& world = sim.GetWorld();
            uint32_t tick = sim.GetState().tickCount;

            int totalTrees = world.treeMatureCount + world.treeYoungCount
                           + world.treeSaplingCount + world.treeEmptySpots
                           + world.treeStumpCount;
            if (tick == 1) {
                m_minTotalTrees = totalTrees;
            }
            if (totalTrees < m_minTotalTrees) {
                m_minTotalTrees = totalTrees;
            }

            // Check for total extinction every 1000 ticks
            if (tick > 0 && tick % 1000 == 0) {
                int mature = world.treeMatureCount;
                int total = world.treeMatureCount + world.treeYoungCount
                          + world.treeSaplingCount + world.treeEmptySpots
                          + world.treeStumpCount;

                if (mature <= 0 && tick > 5000) {
                    printf("[FAIL][T56] Tree extinction at tick %u: mature=%d total=%d\n",
                        tick, mature, total);
                    return false;
                }

                printf("[T56] Tick %5u: mature=%3d young=%3d saplings=%3d stumps=%3d empty=%3d total=%3d (min=%d)\n",
                    tick,
                    world.treeMatureCount, world.treeYoungCount,
                    world.treeSaplingCount, world.treeStumpCount,
                    world.treeEmptySpots, total, m_minTotalTrees);
            }

            if (tick >= 50000) {
                return Verify(sim);
            }

            return true;
        }

        bool Verify(Simulation& sim)
        {
            printf("\n=== T56: Renewable Economy Soak Verification ===\n");

            const WorldModel& world = sim.GetWorld();
            EconomySystem* eco = sim.GetEconomySystem();
            bool ok = true;

            int totalWood = eco ? eco->GetTotalProduced(ResourceType_Wood) : 0;
            int woodFlow = eco ? eco->GetResourceFlow(ResourceType_Wood) : 0;
            int mature = world.treeMatureCount;
            int totalTrees = mature + world.treeYoungCount + world.treeSaplingCount
                           + world.treeStumpCount + world.treeEmptySpots;

            // Count Foresters built by AI
            int foresterCount = 0;
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                if (world.productionBuildings[i].type == BuildingType_Forester) {
                    foresterCount++;
                }
            }

            printf("  Tick:          %u\n", sim.GetState().tickCount);
            printf("  Foresters:     %d\n", foresterCount);
            printf("  Wood produced: %d\n", totalWood);
            printf("  Wood flow:     %d/%d ticks\n", woodFlow, EconomySystem::kFlowWindow);
            printf("  Mature trees:  %d\n", mature);
            printf("  Total trees:   %d\n", totalTrees);
            printf("  Min total:     %d\n", m_minTotalTrees);

            // Invariant 1: Wood was produced
            if (totalWood <= 0) {
                printf("[FAIL][T56] No Wood produced\n");
                ok = false;
            } else {
                printf("[PASS][T56] Wood produced: %d\n", totalWood);
            }

            // Invariant 2: Forest was built
            if (foresterCount == 0) {
                printf("[FAIL][T56] No Forester built\n");
                ok = false;
            } else {
                printf("[PASS][T56] Forester(s) built: %d\n", foresterCount);
            }

            // Invariant 3: Trees survived
            if (mature <= 0) {
                printf("[FAIL][T56] No mature trees remain\n");
                ok = false;
            } else {
                printf("[PASS][T56] Mature trees survive: %d\n", mature);
            }

            // Invariant 4: Total tree ecosystem is non-trivial
            if (totalTrees < 50) {
                printf("[WARN][T56] Total ecosystem shrinking: %d\n", totalTrees);
            }

            if (ok) {
                printf("\n[PASS] T56: Renewable Economy — infinite Wood cycle verified at 50k ticks\n");
                printf("  Forest: %d trees, %d mature, %d Foresters, %d Wood produced\n",
                    totalTrees, mature, foresterCount, totalWood);
            } else {
                printf("\n[FAIL] T56: Renewable Economy cycle broken\n");
            }

            return false;
        }

    private:
        int m_minTotalTrees;
    };

    T56RenewableEconomySoak g_t56;

}
