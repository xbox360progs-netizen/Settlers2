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

class T21WorkerSequentialTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T21"; }

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

        // 3 jobs, each duration 5
        JobManager* jm = sim.GetJobManager();
        if (jm != NULL) {
            jm->CreateJob(JobType_Construction, 1, 0, 5);
            jm->CreateJob(JobType_Production, 2, 1, 5);
            jm->CreateJob(JobType_Construction, 3, 0, 5);
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kTestTicks = 50;

        if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
            printf("[FAIL] T21 failed at tick %u\n", currentTick);
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
            printf("[FAIL][T21] JobManager not available\n");
            return false;
        }

        // Check 1: All 3 jobs completed
        int completedCount = jm->GetCompletedJobCount();
        if (completedCount != 3) {
            printf("[FAIL][T21.A] Expected 3 completed jobs, got %d\n", completedCount);
            ok = false;
        } else {
            printf("[PASS][T21.A] All 3 jobs completed\n");
        }

        // Check 2: No waiting or assigned jobs remain
        int waitingCount = jm->GetWaitingJobCount();
        int assignedCount = jm->GetAssignedJobCount();
        if (waitingCount != 0 || assignedCount != 0) {
            printf("[FAIL][T21.B] Jobs remain: waiting=%d assigned=%d\n", waitingCount, assignedCount);
            ok = false;
        } else {
            printf("[PASS][T21.B] No waiting or assigned jobs remain\n");
        }

        // Check 3: Worker is idle
        const Worker& w = world.workers[0];
        if (w.state != WorkerState_Idle) {
            printf("[FAIL][T21.C] Worker state=%d, expected Idle\n", w.state);
            ok = false;
        } else {
            printf("[PASS][T21.C] Worker returned to Idle\n");
        }

        // Check 4: Worker currentJob is 0
        if (w.currentJob != 0) {
            printf("[FAIL][T21.D] Worker currentJob=%d, expected 0\n", w.currentJob);
            ok = false;
        } else {
            printf("[PASS][T21.D] Worker currentJob reset\n");
        }

        // Check 5: Verify each job is completed individually
        bool allCompleted = true;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state != JobState_Completed) {
                printf("[FAIL][T21.E] Job %d not completed (state=%d)\n", i, job.state);
                allCompleted = false;
                ok = false;
            }
        }
        if (allCompleted) {
            printf("[PASS][T21.E] All jobs individually marked Completed\n");
        }

        if (ok) {
            printf("[PASS] T21: Sequential job execution verified\n");
        }
        return ok;
    }
};

static T21WorkerSequentialTest g_t21WorkerSequentialTest;

} // namespace World
