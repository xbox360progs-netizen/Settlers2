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

        // Lifecycle
        TransportTask* CreateTask(
            ResourceType resource,
            FlagId origin,
            FlagId destination,
            TransportTaskReason reason);

        void CancelTask(TransportTaskId taskId);

        // Event callbacks (from Carrier / world)
        void NotifyCarrierIdle(void* carrier, FlagId atFlag);
        void NotifyCarrierReachedTarget(void* carrier, FlagId flagId);
        void NotifyCarrierPickedUp(void* carrier);
        void NotifyCarrierDropped(void* carrier, FlagId flagId);
        void NotifyRoadNetworkChanged();
        void NotifyFlagRemoved(FlagId flagId);

        // Query
        int GetActiveTaskCount() const;
        TransportTask* GetTaskById(TransportTaskId taskId);

        // Update (empty skeleton until Phase 7.3+)
        void Update(float deltaTime);

    private:
        // Not implemented yet — Phase 7.2+
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;
    };

} // namespace World
