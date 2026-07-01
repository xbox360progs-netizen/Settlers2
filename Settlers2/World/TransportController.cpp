// Phase 7 — Controller. CreateTask + waiting queue + Assigned state.
//
// Self-test scenarios (Phase 7.2.5):
//
//   1. Pool exhaustion:
//      Create 256 tasks with valid routes.
//      257th CreateTask() → returns NULL.
//
//   2. Independent per-flag queues:
//      CreateTask(A→B), CreateTask(A→C), CreateTask(D→E).
//      GetWaitingCount(A) == 2, GetWaitingCount(D) == 1.
//      PeekWaitingTask(A) == first task, PeekWaitingTask(D) == third.
//
//   3. Route truncation at kMaxRouteLength (64):
//      A route longer than 64 flags → first 64 flags stored, rest discarded.
//      Task state == WaitingAtSource (not Blocked).
//
//   4. No-path scenarios:
//      CreateTask between unconnected flags → state == TTS_Blocked.
//      GetBlockedCount() == 1.
//
//   5. Debug API purity:
//      Repeated GetWaitingCount() and PeekWaitingTask() calls with no
//      intervening CreateTask()/CancelTask() return identical results.
//
// Phase 7.3.1 — Assignment scenarios:
//
//   6. One carrier, one task:
//      CreateTask(A→B). NotifyCarrierIdle(c1, A).
//      Task1 state == TTS_Assigned.
//      Carrier1.m_phase7Task == Task1.
//      Waiting queue at A is empty.
//
//   7. No carrier (no NotifyCarrierIdle):
//      CreateTask(A→B). No idle notification.
//      Task1 state == TTS_WaitingAtSource.
//      Waiting queue at A has 1 entry.
//
//   8. No task, idle carrier:
//      NotifyCarrierIdle(c1, A) with empty queue.
//      Nothing changes. Carrier stays idle.
//
//   9. Two carriers, one task:
//      CreateTask(A→B). NotifyCarrierIdle(c1, A). NotifyCarrierIdle(c2, A).
//      Only c1 gets assigned. c2 finds empty queue.
//
//  10. Two tasks, one carrier:
//      CreateTask(A→B), CreateTask(A→C). NotifyCarrierIdle(c1, A).
//      Only Task1 assigned. Task2 remains in queue.

#include <vector>
#include <cassert>
#include "TransportController.h"
#include "TransportTask.h"
#include "RoadManager.h"
#include "FlagManager.h"
#include "Flag.h"
#include "Carrier.h"

namespace World {

    TransportController::TransportController()
        : m_nextTaskId(1)
        , m_activeCount(0)
        , m_roadManager(NULL)
        , m_flagManager(NULL)
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            m_pool[i].id = 0;
            m_pool[i].resource = ResourceType_None;
            m_pool[i].state = TTS_Delivered;
            m_pool[i].cargo = NULL;
            m_pool[i].carrier = NULL;
            m_pool[i].nextWaiting = NULL;
        }
        for (uint32_t i = 0; i < kMaxFlags; ++i) {
            m_waitingHead[i] = NULL;
            m_waitingTail[i] = NULL;
        }
    }

    TransportController::~TransportController() {}

    // ── Pool allocation ──────────────────────────────────────────────────

    TransportTask* TransportController::AllocateTask()
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].state == TTS_Delivered && m_pool[i].id == 0) {
                m_activeCount++;
                return &m_pool[i];
            }
        }
        return NULL;
    }

    // ── Waiting queue ────────────────────────────────────────────────────

    void TransportController::EnqueueWaiting(TransportTask* task, FlagId atFlag)
    {
        assert(atFlag < kMaxFlags);
        assert(task != NULL);
        task->nextWaiting = NULL;

        if (m_waitingTail[atFlag] != NULL) {
            m_waitingTail[atFlag]->nextWaiting = task;
            m_waitingTail[atFlag] = task;
        } else {
            m_waitingHead[atFlag] = task;
            m_waitingTail[atFlag] = task;
        }
    }

    // ── Waiting queue — internal helpers ─────────────────────────────────

    TransportTask* TransportController::PeekWaiting(FlagId flagId) const
    {
        if (flagId >= kMaxFlags) return NULL;
        return m_waitingHead[flagId];
    }

    // AcquireWaitingTask extracts the head task from a waiting queue.
    // Caller must have already validated that the task can be assigned
    // (PeekWaiting + check). Task is only removed once assignment succeeds.
    TransportTask* TransportController::AcquireWaitingTask(FlagId flagId)
    {
        if (flagId >= kMaxFlags) return NULL;
        TransportTask* task = m_waitingHead[flagId];
        if (!task) return NULL;

        m_waitingHead[flagId] = task->nextWaiting;
        if (!m_waitingHead[flagId]) {
            m_waitingTail[flagId] = NULL;
        }
        task->nextWaiting = NULL;
        return task;
    }

    // ── Assignment (Phase 7.3.1) ─────────────────────────────────────────

    // TryAssignTask is the assignment policy:
    //   1. Peek at the waiting queue
    //   2. No task → return NULL (carrier stays idle)
    //   3. Acquire the task (remove from queue)
    //   4. Assign it (set state, link carrier)
    TransportTask* TransportController::TryAssignTask(void* carrier, FlagId atFlag)
    {
        if (!carrier) return NULL;

        TransportTask* task = PeekWaiting(atFlag);
        if (!task) return NULL;

        task = AcquireWaitingTask(atFlag);
        // Peek + Acquire should be consistent — if Peek found one, Acquire must too
        assert(task != NULL);

        AssignTask(carrier, task);
        return task;
    }

    // AssignTask is the single point where Carrier ↔ Task linkage is created.
    // All invariants are checked here.
    void TransportController::AssignTask(void* carrier, TransportTask* task)
    {
        Carrier* c = static_cast<Carrier*>(carrier);
        assert(task->state == TTS_WaitingAtSource);
        assert(task->carrier == NULL);
        assert(c->m_phase7Task == NULL);     // Carrier must be idle

        task->carrier = c;
        task->state = TTS_Assigned;
        // First hop from source — route.flags[0] is the source, flags[1] is the first hop target
        task->targetFlag = task->route.flags[task->hopIndex + 1];

        c->AssignPhase7Task(task, task->targetFlag);

        // Post-conditions
        assert(task->carrier == c);
        assert(task->state == TTS_Assigned);
        assert(task->hopIndex + 1 < task->route.count);
        assert(c->m_phase7Task == task);
        assert(c->m_phase7TargetFlag == task->targetFlag);
    }

    // ── Lifecycle ────────────────────────────────────────────────────────

    TransportTask* TransportController::CreateTask(
        ResourceType resource,
        FlagId origin,
        FlagId destination,
        TransportTaskReason reason)
    {
        if (resource == ResourceType_None) return NULL;
        if (origin == destination) return NULL;

        TransportTask* task = AllocateTask();
        if (!task) return NULL;

        task->id = m_nextTaskId++;
        task->resource = resource;
        task->reason = reason;
        task->state = TTS_Created;
        task->hopIndex = 0;
        task->targetFlag = 0;
        task->cargo = NULL;
        task->carrier = NULL;
        task->createdTick = 0; // updated by caller each frame if needed
        task->nextWaiting = NULL;
        task->priority.classPriority = 0;
        task->priority.dynamicPriority = 0;

        // Build route
        if (m_roadManager && m_flagManager) {
            Flag* srcFlag = m_flagManager->GetFlagById(origin);
            Flag* dstFlag = m_flagManager->GetFlagById(destination);

            if (srcFlag && dstFlag) {
                std::vector<Flag*> path = m_roadManager->FindFlagPath(srcFlag, dstFlag);
                if (path.size() >= 2) {
                    uint8_t count = (path.size() <= kMaxRouteLength)
                        ? (uint8_t)path.size()
                        : kMaxRouteLength;
                    task->route.count = count;
                    for (uint8_t i = 0; i < count; ++i) {
                        task->route.flags[i] = path[i]->id;
                    }
                    task->state = TTS_WaitingAtSource;
                    EnqueueWaiting(task, origin);
                } else {
                    task->state = TTS_Blocked;
                }
            } else {
                task->state = TTS_Blocked;
            }
        } else {
            task->state = TTS_Blocked;
        }

        return task;
    }

    void TransportController::CancelTask(TransportTaskId /*taskId*/) {}

    // ── Event callbacks (stubs until Phase 7.3+) ─────────────────────────

    void TransportController::NotifyCarrierIdle(void* carrier, FlagId atFlag)
    {
        // Carrier became idle at a flag. Try to assign the next waiting task.
        TryAssignTask(carrier, atFlag);
    }
    void TransportController::NotifyCarrierArrived(void* /*carrier*/, FlagId /*flagId*/) {}
    void TransportController::NotifyCarrierPickedUp(void* /*carrier*/) {}
    void TransportController::NotifyCarrierDropped(void* /*carrier*/, FlagId /*flagId*/) {}
    void TransportController::NotifyRoadNetworkChanged() {}
    void TransportController::NotifyFlagRemoved(FlagId /*flagId*/) {}

    // ── Query ────────────────────────────────────────────────────────────

    int TransportController::GetActiveTaskCount() const { return m_activeCount; }

    TransportTask* TransportController::GetTaskById(TransportTaskId taskId)
    {
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].id == taskId) return &m_pool[i];
        }
        return NULL;
    }

    void TransportController::Update(float /*deltaTime*/) {}

    // ── Debug / test API ─────────────────────────────────────────────────

    uint16_t TransportController::GetWaitingCount(FlagId flagId) const
    {
        if (flagId >= kMaxFlags) return 0;
        uint16_t count = 0;
        TransportTask* t = m_waitingHead[flagId];
        while (t) {
            count++;
            t = t->nextWaiting;
        }
        return count;
    }

    TransportTask* TransportController::PeekWaitingTask(FlagId flagId) const
    {
        if (flagId >= kMaxFlags) return NULL;
        return m_waitingHead[flagId];
    }

    uint16_t TransportController::GetBlockedCount() const
    {
        uint16_t count = 0;
        for (int i = 0; i < kMaxTasks; ++i) {
            if (m_pool[i].state == TTS_Blocked) count++;
        }
        return count;
    }

} // namespace World
