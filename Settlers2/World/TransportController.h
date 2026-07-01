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

    // Phase 8.3B — reason tag for delta telemetry
    enum DeltaReason {
        DR_None,
        DR_TaskCreated,
        DR_Pickup,
        DR_AdvanceHop,
        DR_Delivered,
        DR_Cancelled,
        DR_RetryBlocked,
        DR_FlagRemoved
    };

    static const int kMaxTasks = 256;

    class TransportController {
    public:
        TransportController();
        ~TransportController();

        // Phase 8.3B — lightweight economic snapshot for validation
        struct EconomySnapshot {
            uint32_t totalResources;    // flag slots + task cargo
            uint16_t flagInvHash;       // FNV-1a rolling hash of (type, amount) per slot
            uint16_t taskCargoHash;     // FNV-1a rolling hash of (type, amount) per task
            uint32_t ownershipHash;     // FNV-1a of owner category per resource (1=Flag, 2=Task, 4=Ground, 8=Building)
            uint16_t ownershipMask;     // bitmask of categories present (diagnostic)
            uint16_t blockedCount;
            uint16_t flagCount;

            bool operator==(const EconomySnapshot& o) const {
                return totalResources == o.totalResources &&
                       flagInvHash == o.flagInvHash &&
                       taskCargoHash == o.taskCargoHash &&
                       ownershipHash == o.ownershipHash &&
                       ownershipMask == o.ownershipMask &&
                       blockedCount == o.blockedCount &&
                       flagCount == o.flagCount;
            }
            bool operator!=(const EconomySnapshot& o) const { return !(*this == o); }
        const char* m_deltaReason;  // reason tag for next telemetry delta
    };

        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetCarrierManager(CarrierManager* cm) { m_carrierManager = cm; }
        void SetCargoManager(CargoManager* cm) { m_cargoManager = cm; }
        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }

        // ── Lifecycle ─────────────────────────────────────────────────

        TransportTask* CreateTask(
            ResourceType resource,
            FlagId origin,
            FlagId destination,
            TransportTaskReason reason);

        void CancelTask(TransportTaskId taskId);

        // Phase 8.2 — resolve task by ID (O(kMaxTasks), used during Cargo wiring)
        TransportTask* FindTask(TransportTaskId taskId) {
            for (int i = 0; i < kMaxTasks; ++i) {
                if (m_pool[i].id == taskId)
                    return &m_pool[i];
            }
            return NULL;
        }

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
        // Phase 7.7 — telemetry every kTelemetryInterval ticks
        void Update(float /*deltaTime*/) {
            m_currentTick++;
            if ((m_currentTick % kTelemetryInterval) == 0) {
                LogTelemetry();
            }
        }

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

        // ── Telemetry (Phase 7.7) ──────────────────────────────────────
        static const int kTelemetryInterval = 600; // ticks between logs
        void LogTelemetry();

        // Phase 8.3B — economic snapshot
        EconomySnapshot TakeSnapshot() const;

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
        CarrierManager* m_carrierManager;
        CargoManager* m_cargoManager;
        DemandManager* m_demandManager;

        // Phase 8.3B — previous snapshot for delta detection
        EconomySnapshot m_prevSnapshot;
        bool m_snapshotInitialized;
        DeltaReason m_deltaReason;  // reason tag for next telemetry delta
    };

    // Phase 8.3B — enum → string for telemetry output
    const char* GetDeltaReasonName(DeltaReason reason);

} // namespace World
