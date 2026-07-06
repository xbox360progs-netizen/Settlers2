#include "TestRunner.h"
#include "../SimulationCore/Systems/DemandManager.h"
#include "../SimulationCore/World/WorldModel.h"
#include "../SimulationCore/Transport/TransportNode.h"
#include "../SimulationCore/Transport/TransportTypes.h"

namespace World {

// ─── CompleteDemand / OnTaskCreated ─────────────────────────────────────

TEST(DemandManager_CompleteDemand_DecrementsRemaining) {
    DemandManager dm;
    dm.SetDemand(ResourceType_Wood, 3, 42, 100);
    EXPECT_EQ(3u, dm.GetDemandRemaining(0));

    dm.CompleteDemand(1);
    EXPECT_EQ(2u, dm.GetDemandRemaining(0));

    dm.CompleteDemand(1);
    dm.CompleteDemand(1);
    EXPECT_EQ(0u, dm.GetDemandRemaining(0));
}

TEST(DemandManager_CompleteDemand_InvalidTicket) {
    DemandManager dm;
    dm.SetDemand(ResourceType_Wood, 3, 42, 100);

    dm.CompleteDemand(0);
    EXPECT_EQ(3u, dm.GetDemandRemaining(0));

    dm.CompleteDemand(99);
    EXPECT_EQ(3u, dm.GetDemandRemaining(0));
}

TEST(DemandManager_OnTaskCreated_SetsActiveTask) {
    DemandManager dm;
    dm.SetDemand(ResourceType_Wood, 3, 42, 100);

    dm.OnTaskCreated(0, 1);

    dm.CompleteDemand(1);
    EXPECT_EQ(2u, dm.GetDemandRemaining(0));
}

// ─── Tick — pendingDemand integration ────────────────────────────────────

TEST(DemandManager_Tick_ReadsPendingDemand) {
    DemandManager dm;
    WorldModel world;
    world.transportNodeCount = 1;
    TransportNode& node = world.transportNodes[0];
    node.id = 5;

    node.pendingDemand[0].resource = ResourceType_Wood;
    node.pendingDemand[0].amount = 1;
    node.pendingDemand[0].active = true;
    node.pendingDemand[0].targetFlag = kNodeDemandFlagBase + 5;

    dm.Tick(world);

    EXPECT_EQ(1, dm.GetDemandCount());
    EXPECT_EQ(ResourceType_Wood, dm.GetDemandType(0));
    EXPECT_EQ(1u, dm.GetDemandRemaining(0));
}

TEST(DemandManager_Tick_DoesNotDuplicateActiveDemands) {
    DemandManager dm;
    WorldModel world;
    world.transportNodeCount = 1;
    TransportNode& node = world.transportNodes[0];
    node.id = 5;

    dm.SetDemand(ResourceType_Wood, 3, kNodeDemandFlagBase + 5, 100,
                 DemandOwner_Production, TTR_Production);
    EXPECT_EQ(3u, dm.GetDemandRemaining(0));

    node.pendingDemand[0].resource = ResourceType_Wood;
    node.pendingDemand[0].amount = 1;
    node.pendingDemand[0].active = true;
    node.pendingDemand[0].targetFlag = kNodeDemandFlagBase + 5;

    dm.Tick(world);

    EXPECT_EQ(1, dm.GetDemandCount());
    EXPECT_EQ(3u, dm.GetDemandRemaining(0));
}

TEST(DemandManager_Tick_MultipleNodes) {
    DemandManager dm;
    WorldModel world;
    world.transportNodeCount = 2;

    world.transportNodes[0].id = 0;
    world.transportNodes[0].pendingDemand[0].resource = ResourceType_Wood;
    world.transportNodes[0].pendingDemand[0].amount = 1;
    world.transportNodes[0].pendingDemand[0].active = true;
    world.transportNodes[0].pendingDemand[0].targetFlag = kNodeDemandFlagBase + 0;

    world.transportNodes[1].id = 1;
    world.transportNodes[1].pendingDemand[0].resource = ResourceType_Planks;
    world.transportNodes[1].pendingDemand[0].amount = 1;
    world.transportNodes[1].pendingDemand[0].active = true;
    world.transportNodes[1].pendingDemand[0].targetFlag = kNodeDemandFlagBase + 1;

    dm.Tick(world);

    EXPECT_EQ(2, dm.GetDemandCount());
    EXPECT_EQ(ResourceType_Wood, dm.GetDemandType(0));
    EXPECT_EQ(ResourceType_Planks, dm.GetDemandType(1));
}

TEST(DemandManager_Tick_SkipsInactivePendingDemand) {
    DemandManager dm;
    WorldModel world;
    world.transportNodeCount = 1;
    world.transportNodes[0].id = 0;

    world.transportNodes[0].pendingDemand[0].resource = ResourceType_Wood;
    world.transportNodes[0].pendingDemand[0].amount = 1;
    world.transportNodes[0].pendingDemand[0].active = false;
    world.transportNodes[0].pendingDemand[0].targetFlag = kNodeDemandFlagBase + 0;

    dm.Tick(world);

    EXPECT_EQ(0, dm.GetDemandCount());
}

// ─── PublishTransportRequests ──────────────────────────────────────────

TEST(DemandManager_Publish_CreatesRequest) {
    DemandManager dm;
    WorldModel world;

    dm.SetDemand(ResourceType_Wood, 3, 42, 100, DemandOwner_Production, TTR_Production);
    dm.Tick(world);

    EXPECT_EQ(1, world.pendingRequestCount);
    EXPECT_EQ(ResourceType_Wood, world.pendingRequests[0].resource);
    EXPECT_EQ(42u, world.pendingRequests[0].destination);
    EXPECT_EQ(TTR_Production, world.pendingRequests[0].reason);
    EXPECT_EQ(DemandOwner_Production, world.pendingRequests[0].owner);
    EXPECT_FALSE(world.pendingRequests[0].fulfilled);
}

TEST(DemandManager_Publish_SkipsDemandWithActiveTask) {
    DemandManager dm;
    WorldModel world;

    dm.SetDemand(ResourceType_Wood, 3, 42, 100);
    dm.OnTaskCreated(0, 1);
    dm.Tick(world);

    EXPECT_EQ(0, world.pendingRequestCount);
}

TEST(DemandManager_Publish_SkipsZeroRemaining) {
    DemandManager dm;
    WorldModel world;

    dm.SetDemand(ResourceType_Wood, 0, 42, 100);
    dm.Tick(world);

    EXPECT_EQ(0, world.pendingRequestCount);
}

TEST(DemandManager_Publish_TickNoDemands) {
    DemandManager dm;
    WorldModel world;

    world.pendingRequestCount = 0;
    dm.Tick(world);

    EXPECT_EQ(0, world.pendingRequestCount);
}

// ─── pendingDemand targetFlag range ────────────────────────────────────

TEST(DemandManager_PendingDemand_UsesNodeFlagRange) {
    EXPECT_TRUE(kNodeDemandFlagBase > 250u);
    EXPECT_TRUE(kNodeDemandFlagBase + 64 < 500u);
}

} // namespace World