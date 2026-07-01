#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "TransportTypes.h"
#include "TransportTask.h"

// Phase 7 — Controller owns all logistics decisions.
// Everything else (Carrier, Cargo, Flag) reports events or executes commands.

namespace World {

    class CargoManager;
    class CarrierManager;
    class Carrier;
    class RoadManager;
    class FlagManager;
    class DemandManager;
    struct TransportTask;

    static const int kMaxTasks = 256;

    class TransportController {
    public:
        TransportController();
        ~TransportController();

        // Dependency injection (must be called before CreateTask)
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }

        // ── Lifecycle ─────────────────────────────────────────────────

        TransportTask* CreateTask(
            ResourceType resource,
            FlagId origin,
            FlagId destination,
            TransportTaskReason reason);

        void CancelTask(TransportTaskId taskId);

        // ── Event callbacks (from Carrier / world) ─────────────────────
        // Carrier never modifies TransportTask state. It only notifies.
        // Controller decides all state transitions as a response.
        void NotifyCarrierIdle(void* carrier, FlagId atFlag);
        void NotifyCarrierArrived(void* carrier, FlagId flagId);
        void NotifyCarrierPickedUp(void* carrier, void* cargo);
        void NotifyCarrierDropped(void* carrier, FlagId flagId);
        void NotifyRoadNetworkChanged();
        void NotifyFlagRemoved(FlagId flagId);

        // ── Query ─────────────────────────────────────────────────────

        int GetActiveTaskCount() const;
        TransportTask* GetTaskById(TransportTaskId taskId);

        // Debug / test API
        uint16_t GetWaitingCount(FlagId flagId) const;
        TransportTask* PeekWaitingTask(FlagId flagId) const;
        uint16_t GetBlockedCount() const;

        // Phase 7.4 — per-frame tick counter for age bonus
        void Update(float /*deltaTime*/) { m_currentTick++; }

    private:
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        // ── Pool management ────────────────────────────────────────────
        TransportTask* AllocateTask();
        // SetTaskState transitions the task to a new state and asserts
        // that transitionCount < 64 (catches infinite state loops).
        void SetTaskState(TransportTask* task, TransportTaskState newState);

        // ── Waiting queue ──────────────────────────────────────────────
        void EnqueueWaiting(TransportTask* task, FlagId atFlag);
        // Phase 7.4 — PickNextTask selects the best waiting task at a flag:
        //   (priority DESC, enqueueOrder ASC)
        // Age bonus (computed from createdTick) prevents starvation.
        // Returns NULL if queue is empty.
        TransportTask* PickNextTask(FlagId flagId);

        // ── Assignment (Phase 7.3.1) ───────────────────────────────────
        TransportTask* TryAssignTask(void* carrier, FlagId atFlag);
        // AssignTask is the single point where Carrier ↔ Task linkage is created.
        // All invariants are checked here.
        void AssignTask(void* carrier, TransportTask* task);

        // ── Queue management ────────────────────────────────────────────
        void RemoveFromQueue(TransportTask* task);

        // ── Ownership validation ────────────────────────────────────────
        void ValidateAssignment(const TransportTask* task, const Carrier* c) const;
        void ValidateOwnership(const TransportTask* task) const;
        void ValidateMovement(const TransportTask* task) const;

        // ── Retry / recovery ────────────────────────────────────────────
        void RetryBlockedTasks();
        // IsRouteValid checks whether the task's next hop is still reachable.
        // Called from NotifyCarrierArrived before AdvanceHop.
        bool IsRouteValid(const TransportTask* task) const;

        // ── Hop management (Phase 7.3.4) ────────────────────────────────
        bool IsLastHop(const TransportTask* task) const;
        void AdvanceHop(Carrier* c, TransportTask* task);
        void CompleteDelivery(Carrier* c, TransportTask* task);

        // ── Data ───────────────────────────────────────────────────────
        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;
        uint32_t m_currentTick;         // per-frame counter for age computation
        uint32_t m_enqueueCounter;      // monotonic counter for FIFO tiebreak

        static const FlagId kMaxFlags = 256;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        RoadManager* m_roadManager;
        FlagManager* m_flagManager;
    };

} // namespace World
