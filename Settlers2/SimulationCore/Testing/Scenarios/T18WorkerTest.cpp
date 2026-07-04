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

class T18WorkerTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T18"; }

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

        // Create 2 workers
        for (int i = 0; i < 2; ++i) {
            if (loadedWorld.workerCount >= kMaxWorkers) break;
            Worker& w = loadedWorld.workers[loadedWorld.workerCount++];
            w.id = static_cast<WorkerId>(i);
            w.state = WorkerState_Idle;
            w.currentJob = 0;
        }

        // Create 3 jobs via JobManager
        JobManager* jm = sim.GetJobManager();
        if (jm != NULL) {
            jm->CreateJob(JobType_Construction, 1, 0);
            jm->CreateJob(JobType_Construction, 2, 1);
            jm->CreateJob(JobType_Production, 3, 0);
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kTestTicks = 20;

        if (currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T18 failed at tick %u\n", currentTick);
                return false;
            }
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
            printf("[FAIL][T18] JobManager not available\n");
            return false;
        }

        // Check 1: Workers exist and are in valid states
        if (world.workerCount != 2) {
            printf("[FAIL][T18.A] Expected 2 workers, got %d\n", world.workerCount);
            ok = false;
        } else {
            printf("[PASS][T18.A] %d workers present\n", world.workerCount);
        }

        // Check 2: Both workers should have jobs (2 jobs for 2 workers)
        int idleCount = 0;
        int findingCount = 0;
        int assignedCount = 0;
        int walkingCount = 0;
        for (int i = 0; i < world.workerCount; ++i) {
            const Worker& w = world.workers[i];
            switch (w.state) {
                case WorkerState_Idle: idleCount++; break;
                case WorkerState_FindingJob: findingCount++; break;
                case WorkerState_Assigned: assignedCount++; break;
                case WorkerState_Walking: walkingCount++; break;
            }
        }

        // After 20 ticks, all workers should have progressed past Idle
        if (idleCount > 0) {
            printf("[FAIL][T18.B] %d workers still Idle after 20 ticks — job acquisition failed\n", idleCount);
            ok = false;
        } else {
            printf("[PASS][T18.B] No idle workers: %d Finding, %d Assigned, %d Walking\n",
                findingCount, assignedCount, walkingCount);
        }

        // Check 3: Job assignment is exclusive — no job has two workers
        bool exclusive = true;
        for (int i = 0; i < jm->GetJobCount() && exclusive; ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state != JobState_Assigned) continue;
            for (int j = i + 1; j < jm->GetJobCount(); ++j) {
                const Job& other = jm->GetJob(j);
                if (other.state == JobState_Assigned && other.worker == job.worker) {
                    printf("[FAIL][T18.C] Worker %d assigned to two jobs (job %d and job %d)\n",
                        job.worker, i, j);
                    exclusive = false;
                    ok = false;
                    break;
                }
            }
        }
        if (exclusive) {
            printf("[PASS][T18.C] Exclusive assignment: no worker has two jobs\n");
        }

        // Check 4: No worker has more than one job (worker side check)
        bool singleJobPerWorker = true;
        for (int i = 0; i < world.workerCount && singleJobPerWorker; ++i) {
            const Worker& w = world.workers[i];
            if (w.state == WorkerState_Idle || w.state == WorkerState_FindingJob) continue;
            int assignedJobs = 0;
            for (int j = 0; j < jm->GetJobCount(); ++j) {
                const Job& job = jm->GetJob(j);
                if (job.state == JobState_Assigned && job.worker == w.id) {
                    assignedJobs++;
                }
            }
            if (assignedJobs > 1) {
                printf("[FAIL][T18.D] Worker %d has %d assigned jobs\n", w.id, assignedJobs);
                singleJobPerWorker = false;
                ok = false;
            }
        }
        if (singleJobPerWorker) {
            printf("[PASS][T18.D] Single job per worker\n");
        }

        // Check 5: At most 2 jobs assigned (only 2 workers)
        int assignedCount2 = 0;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            if (jm->GetJob(i).state == JobState_Assigned) assignedCount2++;
        }
        if (assignedCount2 > 2) {
            printf("[FAIL][T18.E] %d jobs assigned but only 2 workers exist\n", assignedCount2);
            ok = false;
        } else {
            printf("[PASS][T18.E] %d jobs assigned (≤2 workers)\n", assignedCount2);
        }

        // Check 6: At least one job is Waiting (3 jobs, 2 workers)
        int waitingCount = 0;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            if (jm->GetJob(i).state == JobState_Waiting) waitingCount++;
        }
        if (waitingCount == 0) {
            printf("[FAIL][T18.F] No waiting jobs — expected at least 1 (3 jobs, 2 workers)\n");
            ok = false;
        } else {
            printf("[PASS][T18.F] %d job(s) waiting (correct: 3 jobs - 2 workers)\n", waitingCount);
        }

        if (ok) {
            printf("[PASS] T18: Worker v1 — job acquisition invariant verified\n");
        }
        return ok;
    }
};

static T18WorkerTest g_t18WorkerTest;

} // namespace World
