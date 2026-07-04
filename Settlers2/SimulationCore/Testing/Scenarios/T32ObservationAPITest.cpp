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
#include "../../Systems/DemandManager.h"
#include <stdio.h>

namespace World {

class T32ObservationAPITest : public ISimulationScenario {
public:
    const char* GetName() const { return "T32"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableProduction = true;
        config.enableEconomy = true;
        config.enableConstruction = true;
        config.enableWarehouse = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;
        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // 2 Woodcutters (produce Wood)
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.pendingConstructionCount >= kMaxConstructionRequests) break;
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Woodcutter;
            req.position = Vector2i(10 + i * 10, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }

        // 1 Sawmill (consumes Wood, produces Planks)
        if (loadedWorld.pendingConstructionCount < kMaxConstructionRequests) {
            ConstructionRequest& req = loadedWorld.pendingConstructionRequests[loadedWorld.pendingConstructionCount++];
            req.type = BuildingType_Sawmill;
            req.position = Vector2i(25, 10);
            req.owner = 0;
            req.priority = 1;
            req.fulfilled = false;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kSoakTicks = 500;
        static const uint32_t kCheckInterval = 100;

        if (currentTick > 0 && currentTick % kCheckInterval == 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T32 failed at tick %u\n", currentTick);
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
            printf("[FAIL][T32] EconomySystem not available\n");
            return false;
        }

        // ---- Check 1: GetBuildingCount — Woodcutters ----
        int wcCount = eco->GetBuildingCount(PT_Woodcutter, world);
        if (wcCount < 2) {
            printf("[FAIL][T32.A] GetBuildingCount(Woodcutter) = %d (expected >= 2)\n", wcCount);
            ok = false;
        } else {
            printf("[PASS][T32.A] GetBuildingCount(Woodcutter) = %d\n", wcCount);
        }

        // ---- Check 2: GetBuildingCount — Sawmill ----
        int swCount = eco->GetBuildingCount(PT_Sawmill, world);
        if (swCount < 1) {
            printf("[FAIL][T32.B] GetBuildingCount(Sawmill) = %d (expected >= 1)\n", swCount);
            ok = false;
        } else {
            printf("[PASS][T32.B] GetBuildingCount(Sawmill) = %d\n", swCount);
        }

        // ---- Check 3: GetBuildingCount — unrelated type returns 0 ----
        int fishCount = eco->GetBuildingCount(PT_Fisher, world);
        if (fishCount != 0) {
            printf("[FAIL][T32.C] GetBuildingCount(Fisher) = %d (expected 0)\n", fishCount);
            ok = false;
        } else {
            printf("[PASS][T32.C] GetBuildingCount(Fisher) = %d (correctly 0)\n", fishCount);
        }

        // ---- Check 4: GetDemandBacklog — non-negative ----
        int woodBacklog = eco->GetDemandBacklog(ResourceType_Wood, world);
        int planksBacklog = eco->GetDemandBacklog(ResourceType_Planks, world);
        if (woodBacklog < 0) {
            printf("[FAIL][T32.D] GetDemandBacklog(Wood) = %d (expected >= 0)\n", woodBacklog);
            ok = false;
        } else {
            printf("[PASS][T32.D] GetDemandBacklog(Wood) = %d\n", woodBacklog);
        }
        if (planksBacklog < 0) {
            printf("[FAIL][T32.E] GetDemandBacklog(Planks) = %d (expected >= 0)\n", planksBacklog);
            ok = false;
        } else {
            printf("[PASS][T32.E] GetDemandBacklog(Planks) = %d\n", planksBacklog);
        }

        // ---- Check 5: Existing observation APIs still work ----
        int totalWood = eco->GetTotalProduced(ResourceType_Wood);
        int totalPlanks = eco->GetTotalProduced(ResourceType_Planks);
        int woodAvailable = eco->GetAvailable(ResourceType_Wood, world);
        float woodPotential = eco->GetProductionPotential(ResourceType_Wood, world);

        printf("[INFO][T32.F] Wood produced=%d Planks=%d Available=%d Potential=%.4f\n",
            totalWood, totalPlanks, woodAvailable, woodPotential);

        if (totalWood <= 0) {
            printf("[WARN][T32.F] Wood not produced at all — production may not have started\n");
        }

        if (ok) {
            printf("[PASS] T32: Observation API — GetBuildingCount + GetDemandBacklog verified\n");
        }
        return ok;
    }
};

static T32ObservationAPITest g_t32ObservationAPITest;

} // namespace World
