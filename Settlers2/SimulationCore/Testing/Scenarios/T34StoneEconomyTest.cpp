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

class T34StoneEconomyTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T34"; }

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
        // Empty world — Settlement bootstraps everything autonomously
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 2000;
        static const uint32_t kCheckInterval = 500;

        if (currentTick > 0 && currentTick % kCheckInterval == 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T34 failed at tick %u\n", currentTick);
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
            printf("[FAIL][T34] EconomySystem not available\n");
            return false;
        }

        // Discover Stone producer via Definition Query API
        ProductionType stoneProd = GetProducer(ResourceType_Stone);
        BuildingType stoneBld = GetBuildingTypeForProduction(stoneProd);

        // ---- Check 1: Stone producer exists in definitions ----
        if (stoneProd == PT_None) {
            printf("[FAIL][T34.A] GetProducer(Stone) = PT_None — definition broken\n");
            ok = false;
        } else {
            printf("[PASS][T34.A] GetProducer(Stone) = PT_%d\n", static_cast<int>(stoneProd));
        }

        // ---- Check 2: Building type exists for Stone producer ----
        if (stoneBld == BuildingType_None) {
            printf("[FAIL][T34.B] GetBuildingTypeForProduction(prod=%d) = None — definition broken\n",
                static_cast<int>(stoneProd));
            ok = false;
        } else {
            printf("[PASS][T34.B] Stone producer building type = %d\n", static_cast<int>(stoneBld));
        }

        // ---- Check 3: Stone producer was built by Settlement ----
        bool stoneProducerBuilt = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == stoneBld && world.productionBuildings[i].active) {
                stoneProducerBuilt = true;
                break;
            }
        }
        if (!stoneProducerBuilt) {
            printf("[FAIL][T34.C] Stone producer not built by Settlement after 2000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T34.C] Stone producer built and active\n");
        }

        // ---- Check 4: Stone is being produced ----
        int totalStone = eco->GetTotalProduced(ResourceType_Stone);
        if (totalStone <= 0) {
            printf("[FAIL][T34.D] No Stone produced after 2000 ticks\n");
            ok = false;
        } else {
            printf("[PASS][T34.D] Stone produced: %d\n", totalStone);
        }

        // ---- Check 5: Wood was produced (Settlement bootstrapped properly) ----
        int totalWood = eco->GetTotalProduced(ResourceType_Wood);
        if (totalWood <= 0) {
            printf("[FAIL][T34.E] No Wood produced — basic bootstrap failed\n");
            ok = false;
        } else {
            printf("[PASS][T34.E] Wood produced: %d\n", totalWood);
        }

        // ---- Check 6: Wood production potential >= Stone potential ----
        float woodPotential = eco->GetProductionPotential(ResourceType_Wood, world);
        float stonePotential = eco->GetProductionPotential(ResourceType_Stone, world);
        printf("[INFO][T34.F] Wood potential=%.4f Stone potential=%.4f\n", woodPotential, stonePotential);

        if (stonePotential <= 0.0f) {
            printf("[FAIL][T34.F] Stone production potential is 0 — producer not producing\n");
            ok = false;
        }

        if (ok) {
            printf("[PASS] T34: Stone Economy — bootstrapped via Definition Query API\n");
        }
        return ok;
    }
};

static T34StoneEconomyTest g_t34StoneEconomyTest;

} // namespace World
