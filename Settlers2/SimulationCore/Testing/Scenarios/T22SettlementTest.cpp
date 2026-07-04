#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/WorkerTypes.h"
#include "../../Core/JobTypes.h"
#include "../../Core/BuildingTypes.h"
#include "../../Systems/JobManager.h"
#include "../../Settlement/SettlementSystem.h"
#include <stdio.h>

namespace World {

class T22SettlementTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T22"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableWorkers = true;
        config.enableSettlement = true;
    }

    void Initialize(Simulation& sim)
    {

        WorldModel world;
        world.width = 50;
        world.height = 50;

        // No Woodcutter built, no construction site, no pending jobs
        // Settlement should observe this and publish a BuildWoodcutter Job

        sim.LoadWorld(world);
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kTestTicks = 10;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T22 failed at tick %u\n", currentTick);
            return false;
        }

        if (currentTick >= kTestTicks) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        JobManager* jm = sim.GetJobManager();
        bool ok = true;

        if (jm == NULL) {
            printf("[FAIL][T22] JobManager not available\n");
            return false;
        }

        // Check 1: Exactly one Job was created
        if (jm->GetJobCount() != 1) {
            printf("[FAIL][T22.A] Expected 1 job, got %d\n", jm->GetJobCount());
            ok = false;
        } else {
            printf("[PASS][T22.A] Settlement created exactly 1 job\n");
        }

        // Check 2: The Job is for Woodcutter construction
        if (jm->GetJobCount() > 0) {
            const Job& job = jm->GetJob(0);
            if (job.type != JobType_Construction) {
                printf("[FAIL][T22.B] Expected JobType_Construction, got %d\n", job.type);
                ok = false;
            } else {
                printf("[PASS][T22.B] Job type is Construction\n");
            }
            if (job.buildingIndex != (uint8_t)BuildingType_Woodcutter) {
                printf("[FAIL][T22.C] Expected buildingIndex=Woodcutter(%d), got %d\n",
                    (uint8_t)BuildingType_Woodcutter, job.buildingIndex);
                ok = false;
            } else {
                printf("[PASS][T22.C] Job buildingIndex is Woodcutter\n");
            }
        }

        // Check 3: No duplicate Job was created (should be only 1 after 10 ticks)
        int jobCount = jm->GetJobCount();
        if (jobCount > 1) {
            printf("[FAIL][T22.D] Settlement created duplicate jobs (%d jobs) — guard failed\n", jobCount);
            ok = false;
        } else {
            printf("[PASS][T22.D] No duplicate jobs — guard conditions prevent re-publishing\n");
        }

        // Check 4: Settlement did not modify anything else in the world
        const WorldModel& world = sim.GetWorld();
        if (world.productionBuildingCount != 0) {
            printf("[FAIL][T22.E] Settlement should not create buildings directly (found %d)\n",
                world.productionBuildingCount);
            ok = false;
        } else {
            printf("[PASS][T22.E] Settlement did not directly create buildings\n");
        }
        if (world.activeSiteCount != 0) {
            printf("[FAIL][T22.F] Settlement should not create construction sites directly (found %d)\n",
                world.activeSiteCount);
            ok = false;
        } else {
            printf("[PASS][T22.F] Settlement did not directly create construction sites\n");
        }
        if (world.pendingRequestCount != 0) {
            printf("[FAIL][T22.G] Settlement should not create transport requests (found %d)\n",
                world.pendingRequestCount);
            ok = false;
        } else {
            printf("[PASS][T22.G] Settlement did not directly create transport requests\n");
        }

        if (ok) {
            printf("[PASS] T22: Settlement AI — first autonomous decision verified\n");
            printf("  Observe → Decide(needs Woodcutter) → Publish(BuildWoodcutter Job) → Wait\n");
        }
        return ok;
    }
};

static T22SettlementTest g_t22SettlementTest;

} // namespace World
