#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/JobTypes.h"
#include "../../Core/BuildingTypes.h"
#include "../../Systems/JobManager.h"
#include "../../Settlement/SettlementSystem.h"
#include <stdio.h>

namespace World {

class T30SettlementStorehouseTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T30"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableWorkers = true;
        config.enableSettlement = true;
        config.enableProduction = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;

        // Add an existing Woodcutter — bootstrap Storehouse rule
        // requires at least one active production building.
        ProductionBuilding& wc = world.productionBuildings[world.productionBuildingCount++];
        wc.type = BuildingType_Woodcutter;
        wc.position = Vector2i(10, 10);
        wc.owner = 0;
        wc.active = true;
        wc.cycleTimer = 0;

        // No Storehouse exists — Settlement should publish a BuildStorehouse Job

        sim.LoadWorld(world);
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kTestTicks = 10;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T30 failed at tick %u\n", currentTick);
            return false;
        }

        if (currentTick >= kTestTicks) {
            bool ok = Verify(sim);
            return false;
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        JobManager* jm = sim.GetJobManager();
        bool ok = true;

        if (jm == NULL) {
            printf("[FAIL][T30] JobManager not available\n");
            return false;
        }

        // Check 1: Exactly one Job was created (BuildStorehouse)
        if (jm->GetJobCount() != 1) {
            printf("[FAIL][T30.A] Expected 1 job, got %d\n", jm->GetJobCount());
            ok = false;
        } else {
            printf("[PASS][T30.A] Settlement created exactly 1 job\n");
        }

        // Check 2: The Job is for Storehouse construction
        if (jm->GetJobCount() > 0) {
            const Job& job = jm->GetJob(0);
            if (job.type != JobType_Construction) {
                printf("[FAIL][T30.B] Expected JobType_Construction, got %d\n", job.type);
                ok = false;
            } else {
                printf("[PASS][T30.B] Job type is Construction\n");
            }
            if (job.buildingIndex != (uint8_t)BuildingType_Storehouse) {
                printf("[FAIL][T30.C] Expected buildingIndex=Storehouse(%d), got %d\n",
                    (uint8_t)BuildingType_Storehouse, job.buildingIndex);
                ok = false;
            } else {
                printf("[PASS][T30.C] Job buildingIndex is Storehouse\n");
            }
        }

        // Check 3: No duplicate Job (guard works)
        if (jm->GetJobCount() > 1) {
            printf("[FAIL][T30.D] Settlement created duplicate jobs (%d jobs) — guard failed\n", jm->GetJobCount());
            ok = false;
        } else {
            printf("[PASS][T30.D] No duplicate jobs — guard conditions prevent re-publishing\n");
        }

        // Check 4: Settlement did not modify anything else in the world
        const WorldModel& world = sim.GetWorld();
        if (world.activeSiteCount != 0) {
            printf("[FAIL][T30.E] Settlement should not create construction sites directly (found %d)\n",
                world.activeSiteCount);
            ok = false;
        } else {
            printf("[PASS][T30.E] Settlement did not directly create construction sites\n");
        }
        if (world.pendingRequestCount != 0) {
            printf("[FAIL][T30.F] Settlement should not create transport requests (found %d)\n",
                world.pendingRequestCount);
            ok = false;
        } else {
            printf("[PASS][T30.F] Settlement did not directly create transport requests\n");
        }

        if (ok) {
            printf("[PASS] T30: Settlement bootstrap Storehouse rule verified\n");
            printf("  Observe → Decide(Woodcutter exists + no Storehouse)\n");
            printf("       → Publish(BuildStorehouse Job) → Wait\n");
        }
        return ok;
    }
};

static T30SettlementStorehouseTest g_t30SettlementStorehouseTest;

} // namespace World
