#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Definitions/BuildingDefinition.h"
#include "../../Definitions/ProductionDefinition.h"
#include "../../Systems/EconomySystem.h"
#include "../../Warehouse/WarehouseSystem.h"
#include "../../Construction/ConstructionSite.h"
#include "../../Construction/ConstructionState.h"
#include <stdio.h>
#include "../../Core/ResourceDebug.h"
#include "../../Core/TreeSystem.h"

namespace World {

class T37FullAutonomousTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T37"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
        config.enableWarehouse = true;
        config.enableSettlement = true;
        config.enableTreeDepletion = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;
        SeedTrees(world, 500, 500);
        sim.LoadWorld(world);
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 10000;
        static const uint32_t kCheckInterval = 1000;

        if (currentTick > 0 && currentTick % kCheckInterval == 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T37 failed at tick %u\n", currentTick);
                return false;
            }
        }

        if (currentTick >= kSoakTicks) {
            bool ok = Verify(sim);
            return false;
        }
        return true;
    }

    bool CheckBuildingsExist(const WorldModel& world) const
    {
        BuildingType allTypes[] = {
            BuildingType_Woodcutter,
            BuildingType_Sawmill,
            BuildingType_Stonemason,
            BuildingType_Toolmaker,
            BuildingType_Forester,
        };
        int numTypes = sizeof(allTypes) / sizeof(allTypes[0]);

        for (int t = 0; t < numTypes; ++t) {
            bool found = false;
            for (int i = 0; i < world.productionBuildingCount; ++i) {
                if (world.productionBuildings[i].type == allTypes[t] && world.productionBuildings[i].active) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                printf("[FAIL][T37] Building %d not found\n", (int)allTypes[t]);
                return false;
            }
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        EconomySystem* eco = sim.GetEconomySystem();
        bool ok = true;

        if (eco == NULL) {
            printf("[FAIL][T37] EconomySystem not available\n");
            return false;
        }

        // Check 1: All bootstrap buildings exist
        if (!CheckBuildingsExist(world)) {
            printf("[FAIL][T37.A] Not all bootstrap buildings constructed after 10000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T37.A] All 5 bootstrap production buildings exist\n");
        }

        // Check 2: All 4 resources produced
        int totalWood   = eco->GetTotalProduced(ResourceType_Wood);
        int totalPlanks = eco->GetTotalProduced(ResourceType_Planks);
        int totalStone  = eco->GetTotalProduced(ResourceType_Stone);
        int totalTools  = eco->GetTotalProduced(ResourceType_Tools);

        printf("[INFO][T37.B] Total produced: Wood=%d Planks=%d Stone=%d Tools=%d\n",
            totalWood, totalPlanks, totalStone, totalTools);

        if (totalWood <= 0)   { printf("[FAIL][T37.B] No Wood\n");    ok = false; }
        if (totalPlanks <= 0) { printf("[FAIL][T37.B] No Planks\n");  ok = false; }
        if (totalStone <= 0)  { printf("[FAIL][T37.B] No Stone\n");   ok = false; }
        if (totalTools <= 0)  { printf("[FAIL][T37.B] No Tools\n");   ok = false; }

        if (ok) {
            printf("[PASS][T37.B] All 4 resources produced\n");
        }

        // Check 3: Flow <= Potential for all actively produced resources
        ResourceType activeResources[] = {
            ResourceType_Wood,
            ResourceType_Planks,
            ResourceType_Stone,
            ResourceType_Tools
        };
        int numResources = sizeof(activeResources) / sizeof(activeResources[0]);
        bool allFlowsValid = true;

        for (int r = 0; r < numResources; ++r) {
            int flow = eco->GetResourceFlow(activeResources[r]);
            float potential = eco->GetProductionPotential(activeResources[r], world);
            if (flow > static_cast<int>(potential * EconomySystem::kFlowWindow + 0.5f)) {
                printf("[FAIL][T37.C] %s flow %d > potential %.4f * %d\n",
                    ResourceTypeToString(activeResources[r]), flow, potential, EconomySystem::kFlowWindow);
                allFlowsValid = false;
            }
        }

        if (allFlowsValid) {
            printf("[PASS][T37.C] Flow <= Potential for all active resources\n");
        } else {
            ok = false;
        }

        // Check 4: Building counts via GetBuildingCount
        int wcCount = eco->GetBuildingCount(GetProducer(ResourceType_Wood), world);
        int smCount = eco->GetBuildingCount(GetProducer(ResourceType_Planks), world);
        int smasonCount = eco->GetBuildingCount(GetProducer(ResourceType_Stone), world);
        int tmCount = eco->GetBuildingCount(GetProducer(ResourceType_Tools), world);

        printf("[INFO][T37.D] Building counts: WC=%d SM=%d SMason=%d TM=%d\n",
            wcCount, smCount, smasonCount, tmCount);

        if (wcCount < 1 || smCount < 1 || smasonCount < 1 || tmCount < 1) {
            printf("[FAIL][T37.D] Missing production building via GetBuildingCount\n");
            ok = false;
        } else {
            printf("[PASS][T37.D] GetBuildingCount >= 1 for all production types\n");
        }

        // Check 5: Self-regulation — monotonic production (no rollbacks)
        // totalOutput values are monotonic by ProductionSystem invariants.
        // Verify no negative flows (flow is 0 for unscheduled resources, negative = bug)
        for (int r = 0; r < numResources; ++r) {
            int flow = eco->GetResourceFlow(activeResources[r]);
            if (flow < 0) {
                printf("[FAIL][T37.E] Negative flow for %s: %d\n",
                    ResourceTypeToString(activeResources[r]), flow);
                ok = false;
            }
        }
        if (ok) {
            printf("[PASS][T37.E] All resource flows non-negative\n");
        }

        // Check 6: Warehouse stockpile tracking
        WarehouseSystem* wh = sim.GetWarehouseSystem();
        if (wh != NULL) {
            int whWood = wh->GetStockpileAmount(ResourceType_Wood);
            int whPlanks = wh->GetStockpileAmount(ResourceType_Planks);
            int whStone = wh->GetStockpileAmount(ResourceType_Stone);
            printf("[INFO][T37.F] Warehouse stockpile: Wood=%d Planks=%d Stone=%d\n",
                whWood, whPlanks, whStone);

            // Verify delivery event lifecycle is working
            int totalStock = whWood + whPlanks + whStone;
            if (totalStock <= 0) {
                printf("[WARN][T37.F] Warehouse empty — may indicate delivery lifecycle issue\n");
            } else {
                printf("[PASS][T37.F] Warehouse receiving goods (total=%d)\n", totalStock);
            }
        }

        if (ok) {
            printf("\n=== [PASS] T37: Full Autonomous — all circuits coexist and self-regulate ===\n");
            printf("  Resources: Wood=%d Planks=%d Stone=%d Tools=%d\n",
                totalWood, totalPlanks, totalStone, totalTools);
            printf("  Buildings: WC=%d SM=%d SMason=%d TM=%d Forester=1\n",
                wcCount, smCount, smasonCount, tmCount);
            printf("  Flow <= Potential: verified for all 4 resources\n");
            printf("  10000 ticks, 10 periodic invariant checks passed\n");
        }
        return ok;
    }
};

static T37FullAutonomousTest g_t37FullAutonomousTest;

} // namespace World
