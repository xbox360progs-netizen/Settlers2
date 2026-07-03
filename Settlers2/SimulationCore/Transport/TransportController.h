#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "TransportTypes.h"
#include "TransportTask.h"

namespace World {

    class CargoManager;
    class CarrierManager;
    class Carrier;
    class RoadManager;
    class FlagManager;
    class DemandManager;
    struct TransportTask;

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

        struct EconomySnapshot {
            uint32_t totalResources;
            uint16_t flagInvHash;
            uint16_t taskCargoHash;
            uint32_t ownershipHash;
            uint16_t ownershipMask;
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
        const char* m_deltaReason;
    };

        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetCarrierManager(CarrierManager* cm) { m_carrierManager = cm; }
        void SetCargoManager(CargoManager* cm) { m_cargoManager = cm; }
        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }

        TransportTask* CreateTask(
            ResourceType resource,
            FlagId origin,
            FlagId destination,
            TransportTaskReason reason);

        void CancelTask(TransportTaskId taskId);

        TransportTask* FindTask(TransportTaskId taskId) {
            for (int i = 0; i < kMaxTasks; ++i) {
                if (m_pool[i].id == taskId)
                    return &m_pool[i];
            }
            return NULL;
        }

        void NotifyCarrierIdle(void* carrier, FlagId atFlag);
        void NotifyCarrierArrived(void* carrier, FlagId flagId);
        void NotifyCarrierPickedUp(void* carrier, void* cargo);
        void NotifyCarrierDropped(void* carrier, FlagId flagId);
        void NotifyRoadNetworkChanged();
        void NotifyFlagRemoved(FlagId flagId);

        int GetActiveTaskCount() const;
        TransportTask* GetTaskById(TransportTaskId taskId);

        uint16_t GetWaitingCount(FlagId flagId) const;
        TransportTask* PeekWaitingTask(FlagId flagId) const;
        uint16_t GetBlockedCount() const;

        void Update(float /*deltaTime*/) {
            m_currentTick++;
            if ((m_currentTick % kTelemetryInterval) == 0) {
                LogTelemetry();
            }
        }

    private:
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        TransportTask* AllocateTask();
        void SetTaskState(TransportTask* task, TransportTaskState newState);

        void EnqueueWaiting(TransportTask* task, FlagId atFlag);
        TransportTask* PickNextTask(FlagId flagId);

        TransportTask* TryAssignTask(void* carrier, FlagId atFlag);
        void AssignTask(void* carrier, TransportTask* task);

        void RemoveFromQueue(TransportTask* task);

        void ValidateAssignment(const TransportTask* task, const Carrier* c) const;
        void ValidateOwnership(const TransportTask* task) const;
        void ValidateMovement(const TransportTask* task) const;

        void RetryBlockedTasks();
        bool IsRouteValid(const TransportTask* task) const;

        bool IsLastHop(const TransportTask* task) const;
        void AdvanceHop(Carrier* c, TransportTask* task);
        void CompleteDelivery(Carrier* c, TransportTask* task);

        static const int kTelemetryInterval = 600;
        void LogTelemetry();

        EconomySnapshot TakeSnapshot() const;

        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;
        uint32_t m_currentTick;
        uint32_t m_enqueueCounter;

        static const FlagId kMaxFlags = 256;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        RoadManager* m_roadManager;
        FlagManager* m_flagManager;
        CarrierManager* m_carrierManager;
        CargoManager* m_cargoManager;
        DemandManager* m_demandManager;

        EconomySnapshot m_prevSnapshot;
        bool m_snapshotInitialized;
        DeltaReason m_deltaReason;
    };

    const char* GetDeltaReasonName(DeltaReason reason);

} // namespace World
