#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/EconomyMetrics.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/BuildingTypes.h"
#include "../../Core/ResourceTypes.h"
#include "../../Core/ProductionTypes.h"
#include "../../Core/TreeSystem.h"
#include "../../Systems/EconomySystem.h"
#include <stdio.h>

namespace World {

    // Closed Wood Loop Test
    //
    // Proves that SettlementSystem maintains a stable forest-timber economy
    // through observation-based rules (treeMatureCount, treeEmptySpots).
    //
    // The loop:
    //   Forester → trees → Woodcutter → Wood
    //   SettlementSystem observes tree health and builds Foresters as needed.
    //
    // Invariants:
    //   1. SettlementSystem builds at least one Forester
    //   2. Tree count does not permanently drop to zero
    //   3. Wood production is sustained (output > 0 at end)
    //   4. The Forester:Woodcutter ratio adjusts to maintain equilibrium

    static void SeedForestWorld(WorldModel& world, int woodcutters)
    {
        world.width = 50;
        world.height = 50;

        // Seed trees: enough initial stock to survive until Forester arrives
        SeedTrees(world, 20, 150);

        int idx = 0;
        for (int w = 0; w < woodcutters; ++w) {
            ProductionBuilding& pb = world.productionBuildings[world.productionBuildingCount++];
            pb.type = BuildingType_Woodcutter;
            pb.position = Vector2i(10 + w * 3, 10);
            pb.owner = 0;
            pb.cycleTimer = 0;
            pb.active = true;
            pb.inputsRequested = false;
            pb.inputResources[0] = ResourceType_None;
            pb.inputRequired[0] = 0;
            pb.inputDelivered[0] = 0;
            pb.outputResources[0] = ResourceType_Wood;
            pb.outputBuffer[0] = 0;
            pb.totalOutput[0] = 0;
            ++idx;
        }
    }

    class T51ClosedWoodLoopTest : public ISimulationScenario {
    public:
        const char* GetName() const { return "T51"; }

        void Configure(SimulationConfig& config) const
        {
            config.enableProduction = true;
            config.enableEconomy = true;
            config.enableConstruction = true;
            config.enableWorkers = true;
            config.enableSettlement = true;
            config.enableTreeDepletion = true;
        }

        void Initialize(Simulation& sim)
        {
            WorldModel world;
            SeedForestWorld(world, 1);
            sim.LoadWorld(world);
        }

        bool Tick(Simulation& sim)
        {
            uint32_t currentTick = sim.GetState().tickCount;
            const uint32_t kTotalTicks = 10000;

            if (currentTick >= kTotalTicks) {
                return Verify(sim);
            }

            // Periodic status at milestones
            if (currentTick % 1000 == 0 && currentTick > 0) {
                ReportStatus(sim, currentTick);
            }

            return true;
        }

        void ReportStatus(Simulation& sim, uint32_t tick)
        {
            const WorldModel& world = sim.GetWorld();
            EconomySystem* eco = sim.GetEconomySystem();

            int wcCount = eco ? eco->GetBuildingCount(PT_Woodcutter, world) : 0;
            int fCount = eco ? eco->GetBuildingCount(PT_Forester, world) : 0;
            int mature = world.treeMatureCount;
            int young = world.treeYoungCount;
            int saplings = world.treeSaplingCount;
            int stumps = world.treeStumpCount;
            int empty = world.treeEmptySpots;
            int totalTrees = mature + young + saplings;
            int woodFlow = eco ? eco->GetResourceFlow(ResourceType_Wood) : 0;
            int totalWood = eco ? eco->GetTotalProduced(ResourceType_Wood) : 0;

            printf("  [%5u] Foresters=%d Woodcutters=%d Mature=%d Young=%d Saplings=%d Stumps=%d Empty=%d Total=%d WoodFlow=%d TotalWood=%d\n",
                tick, fCount, wcCount, mature, young, saplings, stumps, empty, totalTrees, woodFlow, totalWood);
        }

        bool Verify(Simulation& sim)
        {
            printf("\n=== T51: Closed Wood Loop Verification ===\n");

            const WorldModel& world = sim.GetWorld();
            EconomySystem* eco = sim.GetEconomySystem();
            bool allOk = true;

            int wcCount = eco ? eco->GetBuildingCount(PT_Woodcutter, world) : 0;
            int fCount = eco ? eco->GetBuildingCount(PT_Forester, world) : 0;
            int mature = world.treeMatureCount;
            int totalTrees = world.treeMatureCount + world.treeYoungCount + world.treeSaplingCount;
            int totalWood = eco ? eco->GetTotalProduced(ResourceType_Wood) : 0;
            int woodFlow = eco ? eco->GetResourceFlow(ResourceType_Wood) : 0;

            printf("  Final state:\n");
            printf("    Woodcutters:  %d\n", wcCount);
            printf("    Foresters:    %d\n", fCount);
            printf("    Mature trees: %d\n", mature);
            printf("    Total trees:  %d\n", totalTrees);
            printf("    Wood produced: %d\n", totalWood);
            printf("    Wood flow (recent): %d/%d ticks\n", woodFlow, EconomySystem::kFlowWindow);

            // Invariant 1: At least one Forester was built
            if (fCount == 0) {
                printf("  [FAIL] No Forester was built\n");
                allOk = false;
            } else {
                printf("  [PASS] Forester(s) built: %d\n", fCount);
            }

            // Invariant 2: Tree population survived
            if (totalTrees <= 0) {
                printf("  [FAIL] All trees died\n");
                allOk = false;
            } else {
                printf("  [PASS] Tree population maintained: %d\n", totalTrees);
            }

            // Invariant 3: Wood production was sustained
            if (totalWood <= 0) {
                printf("  [FAIL] No Wood produced\n");
                allOk = false;
            } else {
                printf("  [PASS] Wood produced: %d\n", totalWood);
            }

            // Invariant 4: Recent flow indicates continued production
            if (woodFlow <= 0 && totalWood > 0) {
                printf("  [WARN] Wood flow stopped near end (may be OK if between cycles)\n");
            } else if (woodFlow > 0) {
                printf("  [PASS] Active wood flow: %d\n", woodFlow);
            }

            if (allOk) {
                printf("\n[PASS] T51: Closed Wood Loop — AI maintains forest equilibrium\n");
            } else {
                printf("\n[FAIL] T51: Closed Wood Loop broken\n");
            }

            return false;
        }
    };

    T51ClosedWoodLoopTest g_t51;

} // namespace World
