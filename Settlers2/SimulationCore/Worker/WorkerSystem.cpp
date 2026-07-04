#include "WorkerSystem.h"
#include <stddef.h>
#include "../Systems/JobManager.h"
#include "../World/WorldModel.h"

namespace World {

    WorkerSystem::WorkerSystem()
        : m_jobManager(NULL)
        , m_tickCount(0)
        , m_workerCount(0)
        , m_pendingJobEventCount(0)
    {
    }

    WorkerSystem::~WorkerSystem()
    {
    }

    void WorkerSystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        // Ensure worker pool is initialized
        if (m_workerCount == 0 && world.workerCount > 0) {
            m_workerCount = world.workerCount;
        }

        for (int i = 0; i < m_workerCount; ++i) {
            Worker& worker = world.workers[i];
            switch (worker.state) {
                case WorkerState_Idle:
                    ProcessIdleWorker(world, i);
                    break;
                case WorkerState_FindingJob:
                    ProcessFindingJob(world, i);
                    break;
                case WorkerState_Assigned:
                    ProcessAssignedWorker(world, i);
                    break;
                case WorkerState_Walking:
                    ProcessWalkingWorker(world, i);
                    break;
                case WorkerState_Working:
                    ProcessWorkingWorker(world, i);
                    break;
            }
        }
    }

    bool WorkerSystem::ReleaseCurrentJob(WorldModel& world, WorkerId workerId)
    {
        for (int i = 0; i < world.workerCount; ++i) {
            Worker& w = world.workers[i];
            if (w.id != workerId) continue;
            if (w.state == WorkerState_Idle || w.state == WorkerState_FindingJob) return false;

            if (m_jobManager != NULL) {
                m_jobManager->ReleaseJob(w.currentJob);
            }
            w.currentJob = 0;
            w.state = WorkerState_Idle;
            return true;
        }
        return false;
    }

    void WorkerSystem::ProcessIdleWorker(WorldModel& world, int workerIndex)
    {
        Worker& worker = world.workers[workerIndex];
        worker.state = WorkerState_FindingJob;
    }

    void WorkerSystem::ProcessFindingJob(WorldModel& world, int workerIndex)
    {
        Worker& worker = world.workers[workerIndex];
        if (m_jobManager == NULL) return;

        JobId jobId = kInvalidJobId;
        if (m_jobManager->AcquireJob(worker.id, jobId)) {
            worker.currentJob = jobId;
            worker.state = WorkerState_Assigned;
        } else {
            // No jobs available — return to idle
            worker.state = WorkerState_Idle;
        }
    }

    void WorkerSystem::ProcessAssignedWorker(WorldModel& world, int workerIndex)
    {
        Worker& worker = world.workers[workerIndex];
        // Transition to Walking — "en route to job site"
        worker.state = WorkerState_Walking;
    }

    void WorkerSystem::ProcessWalkingWorker(WorldModel& world, int workerIndex)
    {
        Worker& worker = world.workers[workerIndex];
        // For v2, walking completes immediately — worker arrives at destination
        // Copy job duration into worker's countdown
        if (m_jobManager != NULL) {
            const Job& job = m_jobManager->GetJob(worker.currentJob);
            worker.workTicksRemaining = job.duration;
        } else {
            worker.workTicksRemaining = 0;
        }
        worker.state = WorkerState_Working;
    }

    void WorkerSystem::ProcessWorkingWorker(WorldModel& world, int workerIndex)
    {
        Worker& worker = world.workers[workerIndex];
        if (worker.workTicksRemaining > 0) {
            worker.workTicksRemaining--;
        }
        if (worker.workTicksRemaining == 0) {
            // Work complete — notify JobManager
            if (m_jobManager != NULL) {
                JobId completedJobId = worker.currentJob;
                JobType completedJobType = m_jobManager->GetJob(completedJobId).type;
                m_jobManager->CompleteJob(completedJobId);
                CaptureCompletedJob(completedJobId, completedJobType, worker.id);
            }
            worker.currentJob = 0;
            worker.state = WorkerState_Idle;
        }
    }

    void WorkerSystem::CaptureCompletedJob(JobId jobId, JobType jobType, WorkerId worker)
    {
        if (m_pendingJobEventCount >= kMaxJobEvents) return;
        JobEvent& ev = m_pendingJobEvents[m_pendingJobEventCount++];
        ev.type = JET_Completed;
        ev.jobId = jobId;
        ev.jobType = jobType;
        ev.worker = worker;
    }

    void WorkerSystem::CaptureJobEvents(WorldModel& world)
    {
        for (int i = 0; i < m_pendingJobEventCount; ++i) {
            if (world.jobEventCount >= kMaxJobEvents) break;
            world.jobEvents[world.jobEventCount++] = m_pendingJobEvents[i];
        }
        m_pendingJobEventCount = 0;
    }

}
