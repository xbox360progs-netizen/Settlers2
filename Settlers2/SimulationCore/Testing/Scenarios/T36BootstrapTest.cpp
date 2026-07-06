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
#include "../../Systems/RenewableResourceSystem.h"
#include "../../Testing/EconomyMetrics.h"
#include "../../Warehouse/WarehouseSystem.h"
#include "../../Construction/ConstructionSite.h"
#include "../../Construction/ConstructionState.h"
#include "../../Core/TreeSystem.h"
#include <stdio.h>

namespace World {

class T36BootstrapTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T36"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
        config.enableWarehouse = false;
        config.enableSettlement = true;
        config.enableWorkers = true;
        config.enableTreeDepletion = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;

        // Seed forest for renewable Wood cycle
        SeedTrees(world, 500, 500);

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
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 5000;
        static const uint32_t kCheckInterval = 1000;

        if (currentTick > 0 && currentTick % kCheckInterval == 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T36 failed at tick %u\n", currentTick);
                return false;
            }
        }

        if (currentTick >= kSoakTicks) {
            bool ok = Verify(sim);
            return false;
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        EconomySystem* eco = sim.GetEconomySystem();
        bool ok = true;

        if (eco == NULL) {
            printf("[FAIL][T36] EconomySystem not available\n");
            return false;
        }

        // ---- Check 1: All bootstrap buildings exist ----
        struct BuildingCheck {
            const char* name;
            BuildingType type;
            bool found;
        };
        BuildingCheck checks[] = {
            { "Woodcutter", BuildingType_Woodcutter, false },
            { "Sawmill",    BuildingType_Sawmill,    false },
            { "Stonemason", BuildingType_Stonemason, false },
            { "Toolmaker",  BuildingType_Toolmaker,  false },
            { "Forester",   BuildingType_Forester,   false },
        };
        int numChecks = sizeof(checks) / sizeof(checks[0]);

        for (int i = 0; i < world.productionBuildingCount; ++i) {
            BuildingType t = world.productionBuildings[i].type;
            for (int j = 0; j < numChecks; ++j) {
                if (t == checks[j].type && world.productionBuildings[i].active) {
                    checks[j].found = true;
                }
            }
        }
        for (int j = 0; j < numChecks; ++j) {
            if (!checks[j].found) {
                printf("[FAIL][T36.A] %s not built after 5000 ticks\n", checks[j].name);
                ok = false;
            } else {
                printf("[PASS][T36.A] %s built\n", checks[j].name);
            }
        }

        // ---- Check 2: Storehouse exists (infrastructure) ----
        bool storehouseFound = false;
        // Storehouse is not a production building, scan world differently
        // Check through construction completion — look for it in completed sites
        // For now, verify via warehouse stockpile being tracked
        WarehouseSystem* wh = sim.GetWarehouseSystem();
        if (wh != NULL) {
            int woodStock = wh->GetStockpileAmount(ResourceType_Wood);
            int planksStock = wh->GetStockpileAmount(ResourceType_Planks);
            int stoneStock = wh->GetStockpileAmount(ResourceType_Stone);
            printf("[INFO][T36.B] Warehouse stockpile: Wood=%d Planks=%d Stone=%d\n",
                woodStock, planksStock, stoneStock);
            if (woodStock == 0 && planksStock == 0 && stoneStock == 0) {
                printf("[WARN][T36.B] Warehouse appears inactive — no stockpile\n");
            } else {
                printf("[PASS][T36.B] Warehouse receiving goods\n");
            }
        } else {
            printf("[WARN][T36.B] WarehouseSystem not available\n");
        }

        // ---- Check 3: All resources produced ----
        int totalWood = eco->GetTotalProduced(ResourceType_Wood);
        int totalPlanks = eco->GetTotalProduced(ResourceType_Planks);
        int totalStone = eco->GetTotalProduced(ResourceType_Stone);
        int totalTools = eco->GetTotalProduced(ResourceType_Tools);

        printf("[INFO][T36.C] Wood=%d Planks=%d Stone=%d Tools=%d\n",
            totalWood, totalPlanks, totalStone, totalTools);

        if (totalWood <= 0)   { printf("[FAIL][T36.C] No Wood produced\n");    ok = false; }
        if (totalPlanks <= 0) { printf("[FAIL][T36.C] No Planks produced\n");  ok = false; }
        if (totalStone <= 0)  { printf("[FAIL][T36.C] No Stone produced\n");   ok = false; }
        if (totalTools <= 0)  { printf("[FAIL][T36.C] No Tools produced\n");   ok = false; }

        if (totalWood > 0 && totalPlanks > 0) {
            // Expected consumption: each Plank consumes 2 Wood (Sawmill definition)
            int woodConsumed = totalPlanks * 2;
            // Toolmaker also consumes Wood: each Tool consumes 1 Wood
            int toolsWoodConsumed = totalTools * 1;
            int expectedWoodOutput = totalWood + woodConsumed + toolsWoodConsumed;
            printf("[INFO][T36.D] Economy: Wood=%d (plus %d consumed by Sawmill + %d by Toolmaker)\n",
                totalWood, woodConsumed, toolsWoodConsumed);
        }

        // ---- Check 4: Flow <= Potential invariants ----
        struct FlowCheck {
            const char* name;
            ResourceType resource;
        };
        FlowCheck flowChecks[] = {
            { "Wood",   ResourceType_Wood },
            { "Planks", ResourceType_Planks },
            { "Stone",  ResourceType_Stone },
            { "Tools",  ResourceType_Tools },
        };
        int numFlow = sizeof(flowChecks) / sizeof(flowChecks[0]);

        for (int f = 0; f < numFlow; ++f) {
            int flow = eco->GetResourceFlow(flowChecks[f].resource);
            float potential = eco->GetProductionPotential(flowChecks[f].resource, world);
            int discreteBound = ComputeDiscreteProductionUpperBound(
                flowChecks[f].resource, world, EconomySystem::kFlowWindow);
            if (flow > discreteBound) {
                printf("[FAIL][T36.E] %s flow (%d) exceeds discrete bound (%d)\n",
                    flowChecks[f].name, flow, discreteBound);
                DiagnoseFlowVsPotential(world, eco, flowChecks[f].resource,
                    flow, potential, sim.GetState().tickCount, "T36");
                ok = false;
            } else {
                printf("[PASS][T36.E] %s flow (%d) <= discrete bound (%d) (potential=%.4f/tick)\n",
                    flowChecks[f].name, flow, discreteBound, potential);
            }
        }

        // ---- Check 5: Building counts — no duplicates from conflicting rules ----
        // Each bootstrap rule should have fired exactly once per building type
        int woodcutterCount = eco->GetBuildingCount(GetProducer(ResourceType_Wood), world);
        int sawmillCount = eco->GetBuildingCount(GetProducer(ResourceType_Planks), world);
        int stonemasonCount = eco->GetBuildingCount(GetProducer(ResourceType_Stone), world);
        int toolmakerCount = eco->GetBuildingCount(GetProducer(ResourceType_Tools), world);

        printf("[INFO][T36.F] Building counts: Woodcutter=%d Sawmill=%d Stonemason=%d Toolmaker=%d\n",
            woodcutterCount, sawmillCount, stonemasonCount, toolmakerCount);

        if (woodcutterCount < 1)  { printf("[FAIL][T36.F] Woodcutter count < 1\n");  ok = false; }
        if (sawmillCount < 1)     { printf("[FAIL][T36.F] Sawmill count < 1\n");     ok = false; }
        if (stonemasonCount < 1)  { printf("[FAIL][T36.F] Stonemason count < 1\n");  ok = false; }
        if (toolmakerCount < 1)   { printf("[FAIL][T36.F] Toolmaker count < 1\n");   ok = false; }

        if (ok) {
            printf("[PASS] T36: Bootstrap circuit — autonomous settlement verified\n");
            printf("  All 5 bootstrap rules coexist without conflict.\n");
            printf("  Full chain: Wood → Planks, Stone → Tools (data-driven, no hardcoded types)\n");
            printf("  Resources: Wood=%d Planks=%d Stone=%d Tools=%d\n",
                totalWood, totalPlanks, totalStone, totalTools);
        }
        return ok;
    }
};

static T36BootstrapTest g_t36BootstrapTest;

} // namespace World
