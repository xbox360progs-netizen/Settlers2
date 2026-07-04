#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"
#include "../Core/WorkerTypes.h"
#include "../Core/JobTypes.h"

namespace World {

    class JobManager;
    struct WorldModel;

    class WorkerSystem : public ISimulationSystem {
    public:
        WorkerSystem();
        ~WorkerSystem();

        void SetJobManager(JobManager* jm) { m_jobManager = jm; }
        virtual void Tick(WorldModel& world);

        // Called by Simulation after system loop, before next tick's consumer stage
        void CaptureJobEvents(WorldModel& world);

        // Testability
        int GetWorkerCount() const { return m_workerCount; }
        bool ReleaseCurrentJob(WorldModel& world, WorkerId workerId);

    private:
        void ProcessIdleWorker(WorldModel& world, int workerIndex);
        void ProcessFindingJob(WorldModel& world, int workerIndex);
        void ProcessAssignedWorker(WorldModel& world, int workerIndex);
        void ProcessWalkingWorker(WorldModel& world, int workerIndex);
        void ProcessWorkingWorker(WorldModel& world, int workerIndex);

        void CaptureCompletedJob(JobId jobId, JobType jobType, WorkerId worker);

        JobManager* m_jobManager;
        uint32_t m_tickCount;
        int m_workerCount;

        // Internal buffer — jobs completed during Tick(), flushed to WorldModel by CaptureJobEvents()
        JobEvent m_pendingJobEvents[kMaxJobEvents];
        int m_pendingJobEventCount;
    };

}
