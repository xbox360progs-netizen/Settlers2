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
#include "../../Worker/WorkerSystem.h"
#include <stdio.h>

namespace World {

class T19WorkerReleaseTest : public ISimulationScenario {
public:
    const char* GetName() const { return "T19"; }

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

        // Create 1 worker
        if (loadedWorld.workerCount < kMaxWorkers) {
            Worker& w = loadedWorld.workers[loadedWorld.workerCount++];
            w.id = 0;
            w.state = WorkerState_Idle;
            w.currentJob = 0;
        }

        // Create 2 jobs
        JobManager* jm = sim.GetJobManager();
        if (jm != NULL) {
            jm->CreateJob(JobType_Construction, 1, 0);
            jm->CreateJob(JobType_Production, 2, 1);
        }
    }

    bool Tick(Simulation& sim)
    {
        uint32_t currentTick = sim.GetState().tickCount;
        static const uint32_t kPhase1Ticks = 10;  // acquire first job
        static const uint32_t kPhase2Ticks = 5;   // after release
        static const uint32_t kTotalTicks = kPhase1Ticks + kPhase2Ticks + 10; // final acquisition

        if (currentTick > 0) {
            if (!Assert::AllInvariants(sim.GetWorld(), currentTick)) {
                printf("[FAIL] T19 failed at tick %u\n", currentTick);
                return false;
            }
        }

        // Phase 2: after phase 1, release the worker's job
        if (currentTick == kPhase1Ticks) {
            WorkerSystem* ws = GetWorkerSystem(sim);
            if (ws != NULL) {
                bool released = ws->ReleaseCurrentJob(sim.GetWorld(), 0);
                printf("[INFO] T19 tick %u: ReleaseCurrentJob(worker 0) = %s\n",
                    currentTick, released ? "success" : "failed");
            }
        }

        if (currentTick >= kTotalTicks) {
            return Verify(sim);
        }
        return true;
    }

    bool Verify(Simulation& sim) {
        const WorldModel& world = sim.GetWorld();
        JobManager* jm = sim.GetJobManager();
        bool ok = true;

        if (jm == NULL) {
            printf("[FAIL][T19] JobManager not available\n");
            return false;
        }

        // Check 1: Worker exists
        if (world.workerCount != 1) {
            printf("[FAIL][T19.A] Expected 1 worker, got %d\n", world.workerCount);
            ok = false;
        } else {
            printf("[PASS][T19.A] 1 worker present\n");
        }

        const Worker& w = world.workers[0];

        // Check 2: Worker is not idle (should have re-acquired a job)
        if (w.state == WorkerState_Idle) {
            printf("[FAIL][T19.B] Worker stuck in Idle after release + re-acquire\n");
            ok = false;
        } else if (w.state == WorkerState_Assigned || w.state == WorkerState_Walking) {
            printf("[PASS][T19.B] Worker reassigned after release: state=%d\n", w.state);
        } else {
            printf("[PASS][T19.B] Worker state after cycle: %d\n", w.state);
        }

        // Check 3: Only one job is assigned (exclusive)
        int assignedCount = 0;
        JobId assignedJobId = kInvalidJobId;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state == JobState_Assigned) {
                assignedCount++;
                assignedJobId = job.id;
            }
        }

        if (assignedCount != 1) {
            printf("[FAIL][T19.C] Expected exactly 1 assigned job after release+reacquire, got %d\n",
                assignedCount);
            ok = false;
        } else {
            printf("[PASS][T19.C] Exactly 1 job assigned (id=%d)\n", assignedJobId);
        }

        // Check 4: The assigned job matches the worker's currentJob
        if (assignedCount == 1 && w.state != WorkerState_Idle && w.state != WorkerState_FindingJob) {
            if (w.currentJob != assignedJobId) {
                printf("[FAIL][T19.D] Worker.currentJob (%d) != assigned job (%d)\n",
                    w.currentJob, assignedJobId);
                ok = false;
            } else {
                printf("[PASS][T19.D] Worker.currentJob matches assigned job\n");
            }
        }

        // Check 5: No double-assignment
        bool doubleAssignment = false;
        for (int i = 0; i < jm->GetJobCount(); ++i) {
            const Job& job = jm->GetJob(i);
            if (job.state != JobState_Assigned) continue;
            for (int j = i + 1; j < jm->GetJobCount(); ++j) {
                const Job& other = jm->GetJob(j);
                if (other.state == JobState_Assigned && other.worker == job.worker) {
                    doubleAssignment = true;
                    break;
                }
            }
        }
        if (doubleAssignment) {
            printf("[FAIL][T19.E] Double assignment detected after release cycle\n");
            ok = false;
        } else {
            printf("[PASS][T19.E] No double assignment after release cycle\n");
        }

        // Check 6: All jobs still exist (no leak from Release)
        int totalJobs = jm->GetJobCount();
        if (totalJobs != 2) {
            printf("[FAIL][T19.F] Job count changed after release: expected 2, got %d\n", totalJobs);
            ok = false;
        } else {
            printf("[PASS][T19.F] Job count stable: %d\n", totalJobs);
        }

        if (ok) {
            printf("[PASS] T19: Worker v1 — release/reacquire cycle verified\n");
        }
        return ok;
    }

    WorkerSystem* GetWorkerSystem(Simulation& sim) {
        return sim.GetWorkerSystem();
    }
};

static T19WorkerReleaseTest g_t19WorkerReleaseTest;

} // namespace World
