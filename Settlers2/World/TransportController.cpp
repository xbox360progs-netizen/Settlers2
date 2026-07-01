// Phase 7 — Controller. CreateTask + waiting queue + blocked retry.
// No Carrier, no Cargo, no PickUp/Drop.
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

#include <vector>
#include "TransportController.h"
#include "TransportTask.h"
#include "RoadManager.h"
#include "FlagManager.h"
#include "Flag.h"

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

    void TransportController::NotifyCarrierIdle(void* /*carrier*/, FlagId /*atFlag*/) {}
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
