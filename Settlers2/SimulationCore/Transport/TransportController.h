#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "TransportTypes.h"
#include "TransportTask.h"

namespace World {

    class Carrier;
    class IRoadGraph;
    class IFlagInventory;
    class ICargoRepository;
    class IDemandService;

    static const int kMaxTasks = 256;

    class TransportController {
    public:
        TransportController(
            IRoadGraph& roadGraph,
            IFlagInventory& inventory,
            ICargoRepository& cargo,
            IDemandService& demand);
        ~TransportController();

        TransportTask* CreateTask(
            ResourceType resource,
            FlagId origin,
            FlagId destination,
            TransportTaskReason reason);

        void CancelTask(TransportTaskId taskId);
        TransportTask* FindTask(TransportTaskId taskId);
        TransportTask* GetTaskById(TransportTaskId taskId);

        void NotifyCarrierIdle(void* carrier, FlagId atFlag);
        void NotifyCarrierArrived(void* carrier, FlagId flagId);
        void NotifyCarrierPickedUp(void* carrier, void* cargo);
        void NotifyCarrierDropped(void* carrier, FlagId flagId);
        void NotifyRoadNetworkChanged();
        void NotifyFlagRemoved(FlagId flagId);

        uint16_t GetWaitingCount(FlagId flagId) const;
        TransportTask* PeekWaitingTask(FlagId flagId) const;
        uint16_t GetBlockedCount() const;
        int GetActiveTaskCount() const;

        struct DeliveryRecord {
            ResourceType resource;
            FlagId destinationFlag;
            TransportTaskReason reason;
        };

        int GetRecentDeliveryCount() const { return m_recentDeliveryCount; }
        const DeliveryRecord& GetRecentDelivery(int index) const { return m_recentDeliveries[index]; }
        void ClearRecentDeliveries() { m_recentDeliveryCount = 0; }

        void Update(float deltaTime);

    private:
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        TransportTask* AllocateTask();
        void SetTaskState(TransportTask* task, TransportTaskState newState);

        void EnqueueWaiting(TransportTask* task, FlagId atFlag);
        TransportTask* PickNextTask(FlagId flagId);
        void RemoveFromQueue(TransportTask* task);

        TransportTask* TryAssignTask(void* carrier, FlagId atFlag);
        void AssignTask(void* carrier, TransportTask* task);

        void ValidateAssignment(const TransportTask* task, const Carrier* c) const;
        void ValidateOwnership(const TransportTask* task) const;
        void ValidateMovement(const TransportTask* task) const;

        void RetryBlockedTasks();
        bool IsRouteValid(const TransportTask* task) const;
        bool IsLastHop(const TransportTask* task) const;
        void AdvanceHop(Carrier* c, TransportTask* task);
        void CompleteDelivery(Carrier* c, TransportTask* task);

        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;
        uint32_t m_currentTick;
        uint32_t m_enqueueCounter;

        static const int kMaxRecentDeliveries = 16;

        static const FlagId kMaxFlags = 256;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        DeliveryRecord m_recentDeliveries[kMaxRecentDeliveries];
        int m_recentDeliveryCount;

        IRoadGraph& m_roads;
        IFlagInventory& m_inventory;
        ICargoRepository& m_cargo;
        IDemandService& m_demand;
    };

} // namespace World
