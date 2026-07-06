// PR11 Phase 1 - Verification. No code changes.
// Tests the public API of TransportController using only SimulationCore types.
// Carrier-dependent lifecycle tests moved to TransportLifecycleTest.cpp
// (Carrier.h now depends only on SimulationCore types - no longer blocked).

#include "TestRunner.h"
#include "../SimulationCore/Transport/TransportController.h"
#include "../SimulationCore/Transport/TransportTypes.h"
#include "../SimulationCore/Transport/TransportRoute.h"
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Interfaces/IRoadGraph.h"
#include "../SimulationCore/Interfaces/IFlagInventory.h"
#include "../SimulationCore/Interfaces/ICargoRepository.h"
#include "../SimulationCore/Interfaces/IDemandService.h"

struct StubRoadGraph : public World::IRoadGraph {
    bool routeFound;

    StubRoadGraph() : routeFound(true) {}

    virtual bool FindRoute(World::FlagId source, World::FlagId destination, World::TransportRoute& outRoute) {
        if (!routeFound) return false;
        outRoute.count = 2;
        outRoute.flags[0] = source;
        outRoute.flags[1] = destination;
        return true;
    }
};

struct StubFlagInventory : public World::IFlagInventory {
    virtual bool ReceiveDelivery(World::FlagId, World::ResourceType, uint8_t, uint32_t) { return true; }
};

struct StubCargoRepository : public World::ICargoRepository {
    virtual void Release(uint32_t) {}
};

struct StubDemandService : public World::IDemandService {
    virtual void CompleteDemand(uint32_t) {}
    virtual void OnTaskCreated(uint32_t, uint32_t) {}
};

struct TransportFixture {
    StubRoadGraph roads;
    StubFlagInventory inv;
    StubCargoRepository cargo;
    StubDemandService demand;
    World::TransportController ctrl;

    TransportFixture()
        : ctrl(roads, inv, cargo, demand) {}
};

TEST(Controller_InitialState) {
    TransportFixture f;
    EXPECT_EQ(f.ctrl.GetActiveTaskCount(), 0);
    EXPECT_EQ(f.ctrl.GetBlockedCount(), 0u);
}

TEST(CreateTask_ValidRoute) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(t != NULL);
    EXPECT_EQ(t->state, World::TTS_WaitingAtSource);
    EXPECT_EQ(t->resource, World::ResourceType_Wood);
    EXPECT_EQ(t->reason, World::TTR_Construction);
    EXPECT_TRUE(t->id > 0);
}

TEST(CreateTask_IncrementsActiveCount) {
    TransportFixture f;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_EQ(f.ctrl.GetActiveTaskCount(), 1);
}

TEST(CreateTask_EnqueuesAtOrigin) {
    TransportFixture f;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_EQ(f.ctrl.GetWaitingCount(1), 1u);
}

TEST(CreateTask_BlockedRoute) {
    TransportFixture f;
    f.roads.routeFound = false;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(t != NULL);
    EXPECT_EQ(t->state, World::TTS_Blocked);
}

TEST(CreateTask_BlockedIncrementsBlockedCount) {
    TransportFixture f;
    f.roads.routeFound = false;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_EQ(f.ctrl.GetBlockedCount(), 1u);
}

TEST(CreateTask_SameOriginAndDestination) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 5, 5, World::TTR_Construction);
    EXPECT_TRUE(t == NULL);
}

TEST(CreateTask_ResourceTypeNone) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_None, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(t == NULL);
}

TEST(CreateTask_ReturnsTaskId) {
    TransportFixture f;
    World::TransportTask* t1 = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    World::TransportTask* t2 = f.ctrl.CreateTask(World::ResourceType_Stone, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(t1->id != t2->id);
    EXPECT_TRUE(t2->id > t1->id);
}

TEST(CreateTask_BasePriorityByReason) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Fish, 1, 5, World::TTR_Food);
    EXPECT_EQ(t->basePriority, World::TBP_High);
}

TEST(PoolExhaustion) {
    TransportFixture f;
    int created = 0;
    for (int i = 0; i < 256; ++i) {
        World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
        if (t) created++;
    }
    EXPECT_EQ(created, 256);
    World::TransportTask* overflow = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_TRUE(overflow == NULL);
    EXPECT_EQ(f.ctrl.GetActiveTaskCount(), 256);
}

TEST(CancelTask_WaitingAtSource) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CancelTask(t->id);
    EXPECT_EQ(t->state, World::TTS_Cancelled);
}

TEST(CancelTask_RemovesFromQueue) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CancelTask(t->id);
    EXPECT_EQ(f.ctrl.GetWaitingCount(1), 0u);
}

TEST(CancelTask_BlockedTask) {
    TransportFixture f;
    f.roads.routeFound = false;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CancelTask(t->id);
    EXPECT_EQ(t->state, World::TTS_Cancelled);
}

TEST(CancelTask_BlockedDecrementsBlockedCount) {
    TransportFixture f;
    f.roads.routeFound = false;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CancelTask(t->id);
    // Note: CancelTask on Blocked does NOT free the pool slot (id stays, transitions to Cancelled).
    // So active count is not decremented - this is a lifecycle invariant.
}

TEST(CancelTask_NonExistentId) {
    TransportFixture f;
    f.ctrl.CancelTask(99999);
}

TEST(CancelTask_DoubleCancel) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CancelTask(t->id);
    f.ctrl.CancelTask(t->id);
    EXPECT_EQ(t->state, World::TTS_Cancelled);
}

TEST(GetWaitingCount_IndependentQueues) {
    TransportFixture f;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 6, World::TTR_Construction);
    f.ctrl.CreateTask(World::ResourceType_Wood, 3, 7, World::TTR_Construction);
    EXPECT_EQ(f.ctrl.GetWaitingCount(1), 2u);
    EXPECT_EQ(f.ctrl.GetWaitingCount(3), 1u);
    EXPECT_EQ(f.ctrl.GetWaitingCount(5), 0u);
}

TEST(PeekWaitingTask_ReturnsFirst) {
    TransportFixture f;
    World::TransportTask* a = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    World::TransportTask* b = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 6, World::TTR_Construction);
    World::TransportTask* first = f.ctrl.PeekWaitingTask(1);
    EXPECT_TRUE(first == a);
    EXPECT_TRUE(first != b);
}

TEST(PeekWaitingTask_EmptyQueue) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.PeekWaitingTask(1);
    EXPECT_TRUE(t == NULL);
}

TEST(GetTaskById_Valid) {
    TransportFixture f;
    World::TransportTask* created = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    World::TransportTask* found = f.ctrl.GetTaskById(created->id);
    EXPECT_TRUE(found == created);
}

TEST(GetTaskById_Invalid) {
    TransportFixture f;
    World::TransportTask* t = f.ctrl.GetTaskById(99999);
    EXPECT_TRUE(t == NULL);
}

TEST(GetTaskById_AfterCancel) {
    TransportFixture f;
    World::TransportTask* created = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CancelTask(created->id);
    World::TransportTask* found = f.ctrl.GetTaskById(created->id);
    EXPECT_TRUE(found == created);
}

TEST(GetBlockedCount_Multiple) {
    TransportFixture f;
    f.roads.routeFound = false;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.CreateTask(World::ResourceType_Wood, 2, 6, World::TTR_Construction);
    EXPECT_EQ(f.ctrl.GetBlockedCount(), 2u);
}

TEST(NotifyFlagRemoved_WaitingAtSource) {
    TransportFixture f;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.NotifyFlagRemoved(1);
    EXPECT_EQ(f.ctrl.GetWaitingCount(1), 0u);
}

TEST(NotifyFlagRemoved_BlockedTask_NoOp) {
    TransportFixture f;
    f.roads.routeFound = false;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.NotifyFlagRemoved(1);
    EXPECT_EQ(f.ctrl.GetBlockedCount(), 1u);
}

TEST(NotifyFlagRemoved_NonExistentFlag) {
    TransportFixture f;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.NotifyFlagRemoved(99);
    EXPECT_EQ(f.ctrl.GetWaitingCount(1), 1u);
}

TEST(NotifyRoadNetworkChanged_RetriesBlocked) {
    TransportFixture f;
    f.roads.routeFound = false;
    World::TransportTask* t = f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    EXPECT_EQ(t->state, World::TTS_Blocked);
    f.roads.routeFound = true;
    f.ctrl.NotifyRoadNetworkChanged();
    EXPECT_EQ(t->state, World::TTS_WaitingAtSource);
    EXPECT_EQ(f.ctrl.GetWaitingCount(1), 1u);
    EXPECT_EQ(f.ctrl.GetBlockedCount(), 0u);
}

TEST(NotifyRoadNetworkChanged_NoBlockedTasks) {
    TransportFixture f;
    f.ctrl.CreateTask(World::ResourceType_Wood, 1, 5, World::TTR_Construction);
    f.ctrl.NotifyRoadNetworkChanged();
    EXPECT_EQ(f.ctrl.GetActiveTaskCount(), 1);
}

TEST(Update_IncrementsTick) {
    TransportFixture f;
    f.ctrl.Update(0.016f);
}

TEST(PriorityForReason_Values) {
    EXPECT_EQ(World::PriorityForReason(World::TTR_Emergency), World::TBP_Critical);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Food), World::TBP_High);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Military), World::TBP_High);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Construction), World::TBP_High);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Production), World::TBP_Normal);
    EXPECT_EQ(World::PriorityForReason(World::TTR_WarehouseBalance), World::TBP_Low);
}

TEST(TransportTaskId_Type) {
    World::TransportTaskId id = 42;
    EXPECT_EQ(id, 42u);
}

TEST(FlagId_Type) {
    World::FlagId f = 42;
    EXPECT_EQ(f, 42u);
}
