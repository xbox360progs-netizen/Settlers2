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
//
// Phase 7.3.2 — PickUp:
//
//  11. Assigned → Moving (no Cargo):
//      NotifyCarrierIdle(c1, A) → Assign Task1.
//      NotifyCarrierPickedUp(c1) → Task1 state == TTS_Moving.
//      Carrier unchanged (no Cargo created).
//
// Phase 7.3.3a — PickUp with Cargo ownership:
//
//  12. Full ownership triangle:
//      NotifyCarrierIdle(c1, A) → Assign Task1.
//      NotifyCarrierPickedUp(c1, cargo1) →
//        Task1.cargo == cargo1
//        cargo1.ownerTask == Task1
//        c1.m_phase7Cargo == cargo1
//        Task1.state == TTS_Moving
//      ValidateOwnership(Task1) passes.
//
// Phase 7.3.3b — Walk:
//
//  13. Walk toward targetFlag:
//      Carrier c1: state == TTS_Moving, targetFlag == route.flags[1].
//      c1.Update(dt) → ep moves toward targetFlag.
//      On arrival: asserts fire, NotifyCarrierArrived(c1, targetFlag).
//      Controller validates: ValidateAssignment, ValidateOwnership,
//        ValidateMovement (state==Moving, targetFlag matches).
//
//  14. Carrier never touches TransportTask:
//      During c1.Update(), no task->state, hopIndex, or route changes.
//      Carrier moves ep/walkDir only (spatial movement).
//
// Phase 7.3.4 — AdvanceHop / CompleteDelivery:
//
//  15. Single hop delivery:
//      Carrier c1 arrives at targetFlag B (destination, last hop).
//      NotifyCarrierArrived → IsLastHop==true → CompleteDelivery.
//      task->state == TTS_Delivered.
//      task->cargo == NULL, task->carrier == NULL.
//      carrier->m_phase7Task == NULL (freed).
//
//  16. Intermediate hop (not last):
//      Route A→B→C, carrier arrives at B (not destination).
//      AdvanceHop: hopIndex 0→1, targetFlag B→C.
//      state == TTS_WaitingAtSource.
//      Task re-enqueued at B.
//      carrier released, NotifyCarrierIdle(c, B).
//      TryAssignTask re-assigns same task (next hop to C).
//
//  17. Ownership preserved across hop:
//      After AdvanceHop + re-assign + NotifyCarrierPickedUp:
//      task->cargo unchanged (same cargo object).
//      ValidateOwnership passes.
//      Carrier walks to C.

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

    // ── Ownership validation ─────────────────────────────────────────────

    void TransportController::ValidateAssignment(const TransportTask* task, const Carrier* c) const
    {
        assert(task != NULL);
        assert(c != NULL);
        assert(task->carrier == c);
        assert(c->m_phase7Task == task);
    }

    void TransportController::ValidateOwnership(const TransportTask* task) const
    {
        assert(task != NULL);
        assert(task->carrier != NULL);
        assert(task->cargo != NULL);
        Carrier* c = static_cast<Carrier*>(task->carrier);
        assert(c->m_phase7Cargo == task->cargo);
        assert(task->cargo->ownerTask == task);
    }

    void TransportController::ValidateMovement(const TransportTask* task) const
    {
        assert(task != NULL);
        assert(task->state == TTS_Moving);
        Carrier* c = static_cast<Carrier*>(task->carrier);
        assert(c != NULL);
        assert(task->targetFlag == c->m_phase7TargetFlag);
    }

    // ── Hop management (Phase 7.3.4) ──────────────────────────────────────

    bool TransportController::IsLastHop(const TransportTask* task) const
    {
        assert(task != NULL);
        // The destination is at route.count - 1. We just arrived at
        // route[hopIndex + 1]. If that equals route.count - 1, it's the
        // final destination.
        return (task->hopIndex + 1 >= task->route.count - 1);
    }

    void TransportController::AdvanceHop(Carrier* c, TransportTask* task)
    {
        assert(task->hopIndex + 1 < task->route.count);
        assert(task->cargo != NULL);

        FlagId oldTargetFlag = task->targetFlag;
        FlagId arrivedFlagId = task->route.flags[task->hopIndex + 1];

        // Advance hop index — we are now at the arrived flag
        task->hopIndex++;
        task->targetFlag = task->route.flags[task->hopIndex + 1];
        task->state = TTS_WaitingAtSource;

        // Release carrier — cargo stays at the flag with the task
        task->carrier = NULL;
        c->m_phase7Task = NULL;
        c->m_phase7TargetFlag = 0;
        c->m_phase7Cargo = NULL;
        c->m_cargo = NULL;

        // Re-enqueue at current flag for pickup to next hop
        EnqueueWaiting(task, arrivedFlagId);

        // Post-conditions
        assert(task->state == TTS_WaitingAtSource);
        assert(task->targetFlag != oldTargetFlag);
        assert(task->carrier == NULL);
        assert(task->cargo != NULL);
        assert(c->m_phase7Task == NULL);

#ifdef _DEBUG
        // Runtime trace for debugging
        char dbg[256];
        _snprintf(dbg, sizeof(dbg),
            "[Transport] AdvanceHop task=%u hop=%u/%u flag=%u next=%u\n",
            task->id, task->hopIndex, task->route.count - 1,
            arrivedFlagId, task->targetFlag);
        OutputDebugStringA(dbg);
#endif

        // Carrier becomes idle — check for next hop
        NotifyCarrierIdle(c, arrivedFlagId);
    }

    void TransportController::CompleteDelivery(Carrier* c, TransportTask* task)
    {
        assert(task->cargo != NULL);

        FlagId destFlagId = task->route.flags[task->route.count - 1];

        // Deliver — unlink cargo from task and carrier
        task->cargo->ownerTask = NULL;
        task->cargo = NULL;
        c->m_phase7Cargo = NULL;
        c->m_cargo = NULL;

        task->state = TTS_Delivered;

        // Release carrier — full disconnection
        task->carrier = NULL;
        c->m_phase7Task = NULL;
        c->m_phase7TargetFlag = 0;

        // Post-conditions
        assert(task->state == TTS_Delivered);
        assert(task->cargo == NULL);
        assert(task->carrier == NULL);
        assert(c->m_phase7Task == NULL);

#ifdef _DEBUG
        char dbg[256];
        _snprintf(dbg, sizeof(dbg),
            "[Transport] Delivered task=%u dest=%u\n",
            task->id, destFlagId);
        OutputDebugStringA(dbg);
#endif

        // Carrier becomes idle at destination
        NotifyCarrierIdle(c, destFlagId);
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
        c->m_phase7Controller = this;

        // Post-conditions
        assert(task->state == TTS_Assigned);
        assert(task->hopIndex + 1 < task->route.count);
        assert(c->m_phase7TargetFlag == task->targetFlag);
        ValidateAssignment(task, c);
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
    void TransportController::NotifyCarrierArrived(void* carrier, FlagId flagId)
    {
        if (!carrier) return;
        Carrier* c = static_cast<Carrier*>(carrier);
        TransportTask* task = c->m_phase7Task;
        if (!task) return;
        ValidateAssignment(task, c);
        ValidateOwnership(task);
        ValidateMovement(task);
        assert(flagId == c->m_phase7TargetFlag);
        assert(task->state == TTS_Moving);

        if (IsLastHop(task)) {
            CompleteDelivery(c, task);
        } else {
            AdvanceHop(c, task);
        }
    }
    void TransportController::NotifyCarrierPickedUp(void* carrier, void* cargo)
    {
        if (!carrier || !cargo) return;
        Carrier* c = static_cast<Carrier*>(carrier);
        Cargo* cargoObj = static_cast<Cargo*>(cargo);
        TransportTask* task = c->m_phase7Task;
        if (!task) return;
        ValidateAssignment(task, c);
        assert(task->state == TTS_Assigned);

        // Ownership triangle: link Task ↔ Cargo ↔ Carrier
        task->cargo = cargoObj;
        cargoObj->ownerTask = task;
        c->m_phase7Cargo = cargoObj;

        task->state = TTS_Moving;

        assert(task->state == TTS_Moving);
        ValidateOwnership(task);
    }
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
