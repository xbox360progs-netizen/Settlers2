#pragma once
#include <stdint.h>
#include "ISimulationSystem.h"
#include "../Core/JobTypes.h"
#include "../Core/WorkerTypes.h"

namespace World {

    struct WorldModel;

    class JobManager : public ISimulationSystem {
    public:
        JobManager();
        ~JobManager();

        virtual void Tick(WorldModel& world);

        // Called by domain systems to publish work
        JobId CreateJob(JobType type, uint8_t targetFlag, uint8_t buildingIndex, uint16_t duration = kDefaultJobDuration);

        // Called by WorkerSystem to acquire waiting work
        // Returns true if a job was assigned, false if none available
        bool AcquireJob(WorkerId worker, JobId& outJobId);

        // Called by WorkerSystem to mark job complete
        void CompleteJob(JobId id);

        // Called by WorkerSystem to release job back to pool (worker failed/abandoned)
        void ReleaseJob(JobId id);

        // Testability
        int GetJobCount() const { return m_jobCount; }
        const Job& GetJob(int index) const;
        int GetWaitingJobCount() const;
        int GetAssignedJobCount() const;
        int GetCompletedJobCount() const;

    private:
        JobManager(const JobManager&);
        void operator=(const JobManager&);

        int FindWaitingJob() const;

        static const int kMaxJobs = 64;
        Job m_jobs[kMaxJobs];
        int m_jobCount;
        uint32_t m_tickCount;
    };

}
