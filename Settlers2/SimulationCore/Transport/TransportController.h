#pragma once
#include <stdint.h>
#include "../Core/ResourceTypes.h"
#include "TransportTypes.h"
#include "TransportTask.h"

namespace World {

    struct WorldModel;
    struct TransportNode;
    class Carrier;
    class IRoadGraph;
    class IFlagInventory;
    class ICargoRepository;
    class IDemandService;

    static const int kMaxTasks = 256;
    static const int kMaxCarriers = 32;

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

        const TransportTask* GetTaskPool() const { return m_pool; }
        static int GetPoolSize() { return kMaxTasks; }

        // PR 3.2 — carrier pool dispatching
        void Tick(WorldModel& world);

        int GetIdleCarrierCount() const;
        int GetBusyCarrierCount() const;
        const TransportCarrier* GetCarrierPool() const { return m_carriers; }
        static int GetMaxCarriers() { return kMaxCarriers; }

        // PR 3.3 — carrier execution query (stateless, for tests)
        TransportNode* FindNodeForFlag(WorldModel& world, FlagId flag) const;

    private:
        TransportController(const TransportController&);
        void operator=(const TransportController&);

        TransportTask* AllocateTask();
        void FreeTask(TransportTask* task);
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

        // PR 3.2 — carrier pool assignment
        void ReleaseCarrierForTask(TransportTaskId taskId);
        int TryAssignWaitingTask();

        TransportTask m_pool[kMaxTasks];
        uint32_t m_nextTaskId;
        int m_activeCount;
        uint32_t m_currentTick;
        uint32_t m_enqueueCounter;

        static const int kMaxRecentDeliveries = 64;

        // Covers real game flags (0-255) and transport node flags (400-463)
        static const FlagId kMaxFlags = 512;
        TransportTask* m_waitingHead[kMaxFlags];
        TransportTask* m_waitingTail[kMaxFlags];

        DeliveryRecord m_recentDeliveries[kMaxRecentDeliveries];
        int m_recentDeliveryCount;

        TransportCarrier m_carriers[kMaxCarriers];

        IRoadGraph& m_roads;
        IFlagInventory& m_inventory;
        ICargoRepository& m_cargo;
        IDemandService& m_demand;
    };

} // namespace World
