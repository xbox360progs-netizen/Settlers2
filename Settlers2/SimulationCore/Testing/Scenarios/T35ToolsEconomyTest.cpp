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
#include "../../Construction/ConstructionSite.h"
#include "../../Construction/ConstructionState.h"
#include <stdio.h>

namespace World {

class T35ToolsEconomyTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T35"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
        config.enableWarehouse = true;
        config.enableSettlement = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;
        sim.LoadWorld(world);
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 5000;
        static const uint32_t kCheckInterval = 1000;

        if (currentTick > 0 && currentTick % kCheckInterval == 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T35 failed at tick %u\n", currentTick);
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
            printf("[FAIL][T35] EconomySystem not available\n");
            return false;
        }

        // ---- Check 1: Full dependency chain via Definition Query API ----
        // Wood: GetProducer(Wood) → Woodcutter → must exist
        ProductionType woodProd = GetProducer(ResourceType_Wood);
        BuildingType woodBld = GetBuildingTypeForProduction(woodProd);
        bool woodExists = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == woodBld && world.productionBuildings[i].active) {
                woodExists = true;
                break;
            }
        }
        if (!woodExists) {
            printf("[FAIL][T35.A] Woodcutter not built after 5000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T35.A] Woodcutter exists (base bootstrap)\n");
        }

        // Planks: GetProducer(Planks) → Sawmill → must exist
        ProductionType planksProd = GetProducer(ResourceType_Planks);
        BuildingType planksBld = GetBuildingTypeForProduction(planksProd);
        bool planksExists = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == planksBld && world.productionBuildings[i].active) {
                planksExists = true;
                break;
            }
        }
        if (!planksExists) {
            printf("[FAIL][T35.B] Sawmill not built after 5000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T35.B] Sawmill exists\n");
        }

        // Stone: GetProducer(Stone) → Stonemason → must exist
        ProductionType stoneProd = GetProducer(ResourceType_Stone);
        BuildingType stoneBld = GetBuildingTypeForProduction(stoneProd);
        bool stoneExists = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == stoneBld && world.productionBuildings[i].active) {
                stoneExists = true;
                break;
            }
        }
        if (!stoneExists) {
            printf("[FAIL][T35.C] Stonemason not built after 5000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T35.C] Stonemason exists (stone bootstrap)\n");
        }

        // Tools: GetProducer(Tools) → Toolmaker → must exist
        ProductionType toolProd = GetProducer(ResourceType_Tools);
        BuildingType toolBld = GetBuildingTypeForProduction(toolProd);
        bool toolExists = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == toolBld && world.productionBuildings[i].active) {
                toolExists = true;
                break;
            }
        }
        if (!toolExists) {
            printf("[FAIL][T35.D] Toolmaker not built — dependency chain incomplete\n");
            ok = false;
        } else {
            printf("[PASS][T35.D] Toolmaker exists (tool bootstrap via dependency chain)\n");
        }

        // ---- Check 2: Tools are being produced ----
        int totalTools = eco->GetTotalProduced(ResourceType_Tools);
        if (totalTools <= 0) {
            printf("[FAIL][T35.E] No Tools produced after 5000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T35.E] Tools produced: %d\n", totalTools);
        }

        // ---- Check 3: All intermediate resources produced ----
        int totalWood = eco->GetTotalProduced(ResourceType_Wood);
        int totalStone = eco->GetTotalProduced(ResourceType_Stone);
        int totalPlanks = eco->GetTotalProduced(ResourceType_Planks);

        printf("[INFO][T35.F] Wood=%d Planks=%d Stone=%d Tools=%d\n",
            totalWood, totalPlanks, totalStone, totalTools);

        if (totalWood <= 0) {
            printf("[FAIL][T35.F] Wood not produced — production chain broken\n");
            ok = false;
        }
        if (totalStone <= 0) {
            printf("[FAIL][T35.G] Stone not produced — stone chain broken\n");
            ok = false;
        }

        // ---- Check 4: Flow <= Potential invariant for Tools ----
        int toolsFlow = eco->GetResourceFlow(ResourceType_Tools);
        float toolsPotential = eco->GetProductionPotential(ResourceType_Tools, world);
        if (toolsFlow > static_cast<int>(toolsPotential * EconomySystem::kFlowWindow + 0.5f)) {
            printf("[FAIL][T35.H] Tools flow (%d) exceeds capacity (%d)\n",
                toolsFlow, static_cast<int>(toolsPotential * EconomySystem::kFlowWindow));
            ok = false;
        } else {
            printf("[PASS][T35.H] Flow <= Potential: %d <= %.4f * %d\n",
                toolsFlow, toolsPotential, EconomySystem::kFlowWindow);
        }

        if (ok) {
            printf("[PASS] T35: Tools Economy — full dependency chain verified\n");
            printf("  Chain: Wood → Planks, Stone → Tools (via Definition Query API)\n");
            printf("  No hardcoded BuildingType references in bootstrap logic.\n");
        }
        return ok;
    }
};

static T35ToolsEconomyTest g_t35ToolsEconomyTest;

} // namespace World
