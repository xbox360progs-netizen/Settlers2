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
#include <stdio.h>

namespace World {

class T24BuildingCompletionCycle : public ISimulationScenario {
public:
    const char* GetName() const { return "T24"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableEconomy = true;
        config.enableWorkers = true;
        config.enableConstruction = true;
        config.enableSettlement = true;
    }

    void Initialize(Simulation& sim)
    {
        WorldModel world;
        world.width = 50;
        world.height = 50;

        sim.LoadWorld(world);

        // Add 1 worker to execute Settlement's BuildWoodcutter Job
        WorldModel& loadedWorld = sim.GetWorld();
        if (loadedWorld.workerCount < kMaxWorkers) {
            Worker& w = loadedWorld.workers[loadedWorld.workerCount++];
            w.id = 0;
            w.state = WorkerState_Idle;
            w.currentJob = 0;
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kTestTicks = 50;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T24 failed at tick %u\n", currentTick);
            return false;
        }

        if (currentTick >= kTestTicks) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        JobManager* jm = sim.GetJobManager();
        bool ok = true;

        if (jm == NULL) {
            printf("[FAIL][T24] JobManager not available\n");
            return false;
        }

        // Check 1: Woodcutter ProductionBuilding exists
        bool foundWoodcutter = false;
        for (int i = 0; i < world.productionBuildingCount; ++i) {
            if (world.productionBuildings[i].type == BuildingType_Woodcutter) {
                foundWoodcutter = true;
                break;
            }
        }
        if (!foundWoodcutter) {
            printf("[FAIL][T24.A] No Woodcutter ProductionBuilding — site→building handoff failed\n");
            ok = false;
        } else {
            printf("[PASS][T24.A] Woodcutter ProductionBuilding exists\n");
        }

        // Check 2: No active construction sites remain (completed sites are converted)
        if (world.activeSiteCount != 0) {
            printf("[FAIL][T24.B] %d active sites remain after completion (expected 0)\n",
                world.activeSiteCount);
            ok = false;
        } else {
            printf("[PASS][T24.B] No active sites remain — all converted to buildings\n");
        }

        // Check 3: Settlement did not create duplicate jobs (guard sees building)
        int jobCount = jm->GetJobCount();
        if (jobCount > 1) {
            printf("[FAIL][T24.C] Settlement created %d jobs — HasBuilding guard failed\n",
                jobCount);
            ok = false;
        } else {
            printf("[PASS][T24.C] No duplicate jobs (HasBuilding guard works)\n");
        }

        if (ok) {
            printf("[PASS] T24: Full building lifecycle verified\n");
            printf("  Settlement→Job→Worker→JobEvent→Construction→Building→Settlement sees\n");
        }
        return ok;
    }
};

static T24BuildingCompletionCycle g_t24BuildingCompletionCycle;

} // namespace World
