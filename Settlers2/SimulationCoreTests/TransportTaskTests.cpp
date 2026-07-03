#include "TestRunner.h"
#include "../SimulationCore/Transport/TransportTask.h"
#include "../SimulationCore/Transport/TransportTypes.h"
#include "../SimulationCore/Core/ResourceTypes.h"

TEST(TransportTask_DefaultConstruction) {
    World::TransportTask t = {};
    EXPECT_EQ(t.id, 0u);
    EXPECT_EQ(t.state, World::TTS_Created);
    EXPECT_EQ(t.resource, World::ResourceType_None);
    EXPECT_EQ(t.hopIndex, 0u);
    EXPECT_TRUE(t.cargo == NULL);
    EXPECT_TRUE(t.carrier == NULL);
    EXPECT_TRUE(t.nextWaiting == NULL);
    EXPECT_EQ(t.transitionCount, 0u);
}

TEST(TransportTask_ResetFields) {
    World::TransportTask t = {};
    t.id = 17;
    t.resource = World::ResourceType_Wood;
    t.state = World::TTS_Created;
    t.hopIndex = 1;
    t.targetFlag = 42;
    t.transitionCount = 3;
    EXPECT_EQ(t.id, 17u);
    EXPECT_EQ(t.resource, World::ResourceType_Wood);
    EXPECT_EQ(t.state, World::TTS_Created);
    EXPECT_EQ(t.hopIndex, 1u);
    EXPECT_EQ(t.targetFlag, 42u);
    EXPECT_EQ(t.transitionCount, 3u);
}

TEST(TransportTask_StateTransition) {
    World::TransportTask t = {};
    t.state = World::TTS_Created;
    t.transitionCount = 1;
    EXPECT_EQ(t.state, World::TTS_Created);
    EXPECT_EQ(t.transitionCount, 1);

    t.state = World::TTS_WaitingAtSource;
    t.transitionCount = 2;
    EXPECT_EQ(t.state, World::TTS_WaitingAtSource);
    EXPECT_EQ(t.transitionCount, 2);

    t.state = World::TTS_Assigned;
    t.transitionCount = 3;
    EXPECT_EQ(t.state, World::TTS_Assigned);
    EXPECT_EQ(t.transitionCount, 3);
}

TEST(TransportTask_RouteFlagAccess) {
    World::TransportTask t = {};
    t.route.count = 3;
    t.route.flags[0] = 10;
    t.route.flags[1] = 20;
    t.route.flags[2] = 30;
    EXPECT_EQ(t.route.count, 3u);
    EXPECT_EQ(t.route.flags[0], 10u);
    EXPECT_EQ(t.route.flags[2], 30u);
}

TEST(TransportTask_ObserverTicketDefault) {
    World::TransportTask t = {};
    EXPECT_EQ(t.observerTicketId, 0u);
}

TEST(TransportTask_CreatedTick) {
    World::TransportTask t = {};
    t.createdTick = 500;
    EXPECT_EQ(t.createdTick, 500u);
}

TEST(TransportTask_BasePriority) {
    World::TransportTask t = {};
    t.basePriority = 100;
    EXPECT_EQ(t.basePriority, 100u);
}
