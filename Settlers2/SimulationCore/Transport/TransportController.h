#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "TransportTypes.h"
#include "TransportTask.h"

namespace World {

    class IRoadGraph;
    class IFlagInventory;
    class ICargoRepository;
    class IDemandService;
    class Carrier;
    struct TransportTask;

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

        void Update(float deltaTime) {
            m_currentTick++;
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

        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;
        uint32_t m_currentTick;
        uint32_t m_enqueueCounter;

        static const FlagId kMaxFlags = 256;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        IRoadGraph& m_roads;
        IFlagInventory& m_inventory;
        ICargoRepository& m_cargo;
        IDemandService& m_demand;
    };

} // namespace World
