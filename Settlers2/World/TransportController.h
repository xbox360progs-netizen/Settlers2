#pragma once
#include <stdint.h>
#include "ResourceNode.h"
#include "TransportTypes.h"

// Phase 7 — Controller declaration only. No logic.
// Controller owns all logistics decisions.
// Everything else (Carrier, Cargo, Flag) reports events or executes commands.

namespace World {

    class CargoManager;
    class CarrierManager;
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

        // Lifecycle
        TransportTask* CreateTask(
            ResourceType resource,
            FlagId origin,
            FlagId destination,
            TransportTaskReason reason);

        void CancelTask(TransportTaskId taskId);

        // Event callbacks (from Carrier / world)
        // Carrier never modifies TransportTask state. It only notifies.
        // Controller decides all state transitions as a response.
        void NotifyCarrierIdle(void* carrier, FlagId atFlag);
        void NotifyCarrierArrived(void* carrier, FlagId flagId);
        void NotifyCarrierPickedUp(void* carrier);
        void NotifyCarrierDropped(void* carrier, FlagId flagId);
        void NotifyRoadNetworkChanged();
        void NotifyFlagRemoved(FlagId flagId);

        // Query
        int GetActiveTaskCount() const;
        TransportTask* GetTaskById(TransportTaskId taskId);

        // Debug / test API
        uint16_t GetWaitingCount(FlagId flagId) const;
        TransportTask* PeekWaitingTask(FlagId flagId) const;
        uint16_t GetBlockedCount() const;

        // Update (empty skeleton until Phase 7.3+)
        void Update(float deltaTime);

    private:
        // Not implemented yet
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        // Find free slot in pool, return NULL if full
        TransportTask* AllocateTask();

        // Enqueue task at the given flag's waiting list
        void EnqueueWaiting(TransportTask* task, FlagId atFlag);

        // Pool
        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;

        // Per-flag waiting queues (linked list through TransportTask::nextWaiting)
        static const FlagId kMaxFlags = 256;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        // Dependencies
        RoadManager* m_roadManager;
        FlagManager* m_flagManager;
    };

} // namespace World
