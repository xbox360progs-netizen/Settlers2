#include "TestRunner.h"
#include "../SimulationCore/Transport/TransportRoute.h"
#include "../SimulationCore/Transport/TransportTypes.h"

TEST(TransportRoute_DefaultEmpty) {
    World::TransportRoute r = {};
    EXPECT_EQ(r.count, 0u);
}

TEST(TransportRoute_SetFlags) {
    World::TransportRoute r = {};
    r.count = 3;
    r.flags[0] = 5;
    r.flags[1] = 10;
    r.flags[2] = 15;
    EXPECT_EQ(r.count, 3u);
    EXPECT_EQ(r.flags[0], 5u);
    EXPECT_EQ(r.flags[1], 10u);
    EXPECT_EQ(r.flags[2], 15u);
}

TEST(TransportRoute_MaxLength) {
    World::TransportRoute r = {};
    r.count = World::kMaxRouteLength;
    for (int i = 0; i < World::kMaxRouteLength; ++i)
        r.flags[i] = (World::FlagId)(i * 10);
    EXPECT_EQ(r.count, World::kMaxRouteLength);
    EXPECT_EQ(r.flags[World::kMaxRouteLength - 1], (World::FlagId)((World::kMaxRouteLength - 1) * 10));
}

TEST(TransportRoute_FlagIdType) {
    World::FlagId f = 42;
    EXPECT_EQ(f, 42u);
}

TEST(TransportRoute_PriorityForReason) {
    EXPECT_EQ(World::PriorityForReason(World::TTR_Emergency), World::TBP_Critical);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Food), World::TBP_High);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Military), World::TBP_High);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Construction), World::TBP_Normal);
    EXPECT_EQ(World::PriorityForReason(World::TTR_Production), World::TBP_Normal);
    EXPECT_EQ(World::PriorityForReason(World::TTR_WarehouseBalance), World::TBP_Low);
}
