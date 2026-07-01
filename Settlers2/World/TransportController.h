#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "TransportTypes.h"

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

        // Phase 7.3.1+ — no per-frame update needed (event-driven)
        void Update(float /*deltaTime*/) {}

    private:
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        // ── Pool management ────────────────────────────────────────────
        TransportTask* AllocateTask();

        // ── Waiting queue ──────────────────────────────────────────────
        void EnqueueWaiting(TransportTask* task, FlagId atFlag);
        TransportTask* PeekWaiting(FlagId flagId) const;
        // AcquireWaitingTask extracts the head only after successful assignment.
        // Peek + validate + Acquire prevents popping a task that can't be assigned.
        TransportTask* AcquireWaitingTask(FlagId flagId);

        // ── Assignment (Phase 7.3.1) ───────────────────────────────────
        TransportTask* TryAssignTask(void* carrier, FlagId atFlag);
        // AssignTask is the single point where Carrier ↔ Task linkage is created.
        // All invariants are checked here.
        void AssignTask(void* carrier, TransportTask* task);

        // ── Ownership validation ────────────────────────────────────────
        // Asserts bidirectional link between task and carrier.
        void ValidateAssignment(const TransportTask* task, const Carrier* c) const;
        // Asserts task↔carrier↔cargo ownership triangle.
        void ValidateOwnership(const TransportTask* task) const;

        // ── Data ───────────────────────────────────────────────────────
        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;

        static const FlagId kMaxFlags = 256;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        RoadManager* m_roadManager;
        FlagManager* m_flagManager;
    };

} // namespace World
