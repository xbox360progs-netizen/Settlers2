// PR 3.5 — RoadGraph unit tests.
// Verifies BFS pathfinding, edge management, and routing invariants.

#include "TestRunner.h"
#include "../SimulationCore/Transport/RoadGraph.h"
#include "../SimulationCore/Transport/TransportRoute.h"

namespace World {

TEST(RoadGraph_NoPath) {
    RoadGraph g;
    TransportRoute route;
    EXPECT_FALSE(g.FindRoute(1, 2, route));
}

TEST(RoadGraph_SameSourceDest) {
    RoadGraph g;
    g.AddEdge(1, 2);
    TransportRoute route;
    EXPECT_FALSE(g.FindRoute(1, 1, route));
}

TEST(RoadGraph_SingleHop) {
    RoadGraph g;
    g.AddEdge(1, 2);
    TransportRoute route;
    EXPECT_TRUE(g.FindRoute(1, 2, route));
    EXPECT_EQ(route.count, 2u);
    EXPECT_EQ(route.flags[0], 1u);
    EXPECT_EQ(route.flags[1], 2u);
}

TEST(RoadGraph_TwoHops) {
    RoadGraph g;
    g.AddEdge(1, 2);
    g.AddEdge(2, 3);
    TransportRoute route;
    EXPECT_TRUE(g.FindRoute(1, 3, route));
    EXPECT_EQ(route.count, 3u);
    EXPECT_EQ(route.flags[0], 1u);
    EXPECT_EQ(route.flags[1], 2u);
    EXPECT_EQ(route.flags[2], 3u);
}

TEST(RoadGraph_ThreeHops) {
    RoadGraph g;
    g.AddEdge(10, 20);
    g.AddEdge(20, 30);
    g.AddEdge(30, 40);
    TransportRoute route;
    EXPECT_TRUE(g.FindRoute(10, 40, route));
    EXPECT_EQ(route.count, 4u);
    EXPECT_EQ(route.flags[0], 10u);
    EXPECT_EQ(route.flags[1], 20u);
    EXPECT_EQ(route.flags[2], 30u);
    EXPECT_EQ(route.flags[3], 40u);
}

TEST(RoadGraph_Bidirectional) {
    RoadGraph g;
    g.AddEdge(5, 10);
    TransportRoute route;
    EXPECT_TRUE(g.FindRoute(10, 5, route));
    EXPECT_EQ(route.count, 2u);
    EXPECT_EQ(route.flags[0], 10u);
    EXPECT_EQ(route.flags[1], 5u);
}

TEST(RoadGraph_RemoveEdge) {
    RoadGraph g;
    g.AddEdge(1, 2);
    g.AddEdge(2, 3);
    g.RemoveEdge(1, 2);
    TransportRoute route;
    EXPECT_FALSE(g.FindRoute(1, 3, route));
}

TEST(RoadGraph_HasEdge) {
    RoadGraph g;
    g.AddEdge(1, 2);
    EXPECT_TRUE(g.HasEdge(1, 2));
    EXPECT_TRUE(g.HasEdge(2, 1));
    EXPECT_FALSE(g.HasEdge(1, 3));
    g.RemoveEdge(1, 2);
    EXPECT_FALSE(g.HasEdge(1, 2));
}

TEST(RoadGraph_BFSPicksShortestPath) {
    RoadGraph g;
    // Two paths: 1→2→4 (2 hops) and 1→3→4 (2 hops, same length)
    // Add a shorter path 1→4 directly
    g.AddEdge(1, 4);
    g.AddEdge(1, 2);
    g.AddEdge(2, 4);
    g.AddEdge(1, 3);
    g.AddEdge(3, 4);
    TransportRoute route;
    EXPECT_TRUE(g.FindRoute(1, 4, route));
    // BFS finds shortest path: 1→4 (single hop)
    EXPECT_EQ(route.count, 2u);
    EXPECT_EQ(route.flags[0], 1u);
    EXPECT_EQ(route.flags[1], 4u);
}

TEST(RoadGraph_OutOfRangeFlags) {
    RoadGraph g;
    g.AddEdge(1, 2);
    TransportRoute route;
    EXPECT_FALSE(g.FindRoute(1, 999, route));
    EXPECT_FALSE(g.FindRoute(999, 1, route));
}

} // namespace World
