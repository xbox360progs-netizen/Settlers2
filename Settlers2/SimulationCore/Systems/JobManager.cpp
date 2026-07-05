#include "JobManager.h"
#include "../World/WorldModel.h"

namespace World {

    JobManager::JobManager()
        : m_jobCount(0)
        , m_tickCount(0)
    {
    }

    JobManager::~JobManager()
    {
    }

    void JobManager::Tick(WorldModel& /*world*/)
    {
        ++m_tickCount;
        // Future: clean up stale jobs, republish, etc.
    }

    JobId JobManager::CreateJob(JobType type, uint16_t targetFlag, uint8_t buildingIndex, uint16_t duration)
    {
        if (m_jobCount >= kMaxJobs) return kInvalidJobId;

        JobId id = static_cast<JobId>(m_jobCount);
        Job& job = m_jobs[m_jobCount];
        job.id = id;
        job.type = type;
        job.state = JobState_Waiting;
        job.targetFlag = targetFlag;
        job.buildingIndex = buildingIndex;
        job.worker = kInvalidWorkerId;
        job.duration = duration;

        m_jobCount++;
        return id;
    }

    bool JobManager::AcquireJob(WorkerId worker, JobId& outJobId)
    {
        int idx = FindWaitingJob();
        if (idx < 0) return false;

        Job& job = m_jobs[idx];
        job.state = JobState_Assigned;
        job.worker = worker;
        outJobId = job.id;
        return true;
    }

    void JobManager::CompleteJob(JobId id)
    {
        for (int i = 0; i < m_jobCount; ++i) {
            if (m_jobs[i].id == id) {
                m_jobs[i].state = JobState_Completed;
                m_jobs[i].worker = kInvalidWorkerId;
                break;
            }
        }
    }

    void JobManager::ReleaseJob(JobId id)
    {
        for (int i = 0; i < m_jobCount; ++i) {
            if (m_jobs[i].id == id) {
                m_jobs[i].state = JobState_Waiting;
                m_jobs[i].worker = kInvalidWorkerId;
                break;
            }
        }
    }

    const Job& JobManager::GetJob(int index) const
    {
        static const Job s_invalid;
        if (index < 0 || index >= m_jobCount) return s_invalid;
        return m_jobs[index];
    }

    int JobManager::GetWaitingJobCount() const
    {
        int count = 0;
        for (int i = 0; i < m_jobCount; ++i) {
            if (m_jobs[i].state == JobState_Waiting) count++;
        }
        return count;
    }

    int JobManager::GetAssignedJobCount() const
    {
        int count = 0;
        for (int i = 0; i < m_jobCount; ++i) {
            if (m_jobs[i].state == JobState_Assigned) count++;
        }
        return count;
    }

    int JobManager::GetCompletedJobCount() const
    {
        int count = 0;
        for (int i = 0; i < m_jobCount; ++i) {
            if (m_jobs[i].state == JobState_Completed) count++;
        }
        return count;
    }

    int JobManager::FindWaitingJob() const
    {
        for (int i = 0; i < m_jobCount; ++i) {
            if (m_jobs[i].state == JobState_Waiting) return i;
        }
        return -1;
    }

}
