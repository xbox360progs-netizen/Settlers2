#include "../../Testing/ISimulationScenario.h"
#include "../../Testing/Assertions/ConstructionAssertions.h"
#include "../../Testing/Assertions/TransportAssertions.h"
#include "../../Testing/Assertions/WorldAssertions.h"
#include "../../Simulation/Simulation.h"
#include "../../Simulation/SimulationConfig.h"
#include "../../World/WorldModel.h"
#include "../../Core/WorkerTypes.h"
#include "../../Core/JobTypes.h"
#include "../../Systems/JobManager.h"
#include <stdio.h>

namespace World {

class T20WorkerExecutionTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T20"; }

    void Configure(SimulationConfig& config) const
    {
        config.enableWorkers = true;
    }

    void Initialize(Simulation& sim)
    {

        WorldModel world;
        world.width = 50;
        world.height = 50;
        sim.LoadWorld(world);

        WorldModel& loadedWorld = sim.GetWorld();

        // 1 worker
        if (loadedWorld.workerCount < kMaxWorkers) {
            Worker& w = loadedWorld.workers[loadedWorld.workerCount++];
            w.id = 1;
            w.state = WorkerState_Idle;
            w.currentJob = 0;
            w.workTicksRemaining = 0;
        }

        // 1 job with duration 10
        JobManager* jm = sim.GetJobManager();
        if (jm != NULL) {
            jm->CreateJob(JobType_Construction, 1, 0, 10);
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kTestTicks = 25;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T20 failed at tick %u\n", currentTick);
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
            printf("[FAIL][T20] JobManager not available\n");
            return false;
        }

        // Check 1: Worker is idle (completed the job)
        if (world.workerCount != 1) {
            printf("[FAIL][T20.A] Expected 1 worker, got %d\n", world.workerCount);
            ok = false;
        } else {
            const Worker& w = world.workers[0];
            if (w.state != WorkerState_Idle) {
                printf("[FAIL][T20.A] Expected worker in Idle state, got %d\n", w.state);
                ok = false;
            } else {
                printf("[PASS][T20.A] Worker returned to Idle after completion\n");
            }
        }

        // Check 2: Job is marked Completed
        int waitingCount = jm->GetWaitingJobCount();
        int assignedCount = jm->GetAssignedJobCount();
        int completedCount = jm->GetCompletedJobCount();
        if (waitingCount != 0) {
            printf("[FAIL][T20.B] Expected 0 waiting jobs, got %d\n", waitingCount);
            ok = false;
        }
        if (assignedCount != 0) {
            printf("[FAIL][T20.B] Expected 0 assigned jobs, got %d\n", assignedCount);
            ok = false;
        }
        if (completedCount != 1) {
            printf("[FAIL][T20.B] Expected 1 completed job, got %d\n", completedCount);
            ok = false;
        }
        if (waitingCount == 0 && assignedCount == 0 && completedCount == 1) {
            printf("[PASS][T20.B] Job lifecycle complete: waiting=%d assigned=%d completed=%d\n",
                waitingCount, assignedCount, completedCount);
        }

        // Check 3: Worker's currentJob is reset
        const Worker& w = world.workers[0];
        if (w.currentJob != 0) {
            printf("[FAIL][T20.C] Worker's currentJob not reset (got %d)\n", w.currentJob);
            ok = false;
        } else {
            printf("[PASS][T20.C] Worker currentJob reset to 0\n");
        }

        // Check 4: workTicksRemaining is 0
        if (w.workTicksRemaining != 0) {
            printf("[FAIL][T20.D] workTicksRemaining=%d, expected 0\n", w.workTicksRemaining);
            ok = false;
        } else {
            printf("[PASS][T20.D] workTicksRemaining reset to 0\n");
        }

        if (ok) {
            printf("[PASS] T20: Worker execution lifecycle verified\n");
        }
        return ok;
    }
};

static T20WorkerExecutionTest g_t20WorkerExecutionTest;

} // namespace World
