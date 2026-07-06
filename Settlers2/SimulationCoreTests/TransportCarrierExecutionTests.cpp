// PR 3.3 — TransportController carrier execution tests.
// Verifies the carrier state machine: Idle → Assigned → Pickup → Travelling → Delivering → Idle.
// Uses TransportNode setup for source/destination buffers.
//
// Invariant: carrier completes the full lifecycle in one Tick (immediate travel in v1).
// Tests verify observable side effects (buffer state, delivery events), not transient states.

#include "TestRunner.h"
#include "../SimulationCore/Transport/TransportController.h"
#include "../SimulationCore/Transport/TransportTypes.h"
#include "../SimulationCore/Transport/TransportRoute.h"
#include "../SimulationCore/Transport/TransportNode.h"
#include "../SimulationCore/World/WorldModel.h"
#include "../SimulationCore/Core/ResourceTypes.h"
#include "../SimulationCore/Interfaces/IRoadGraph.h"
#include "../SimulationCore/Interfaces/IFlagInventory.h"
#include "../SimulationCore/Interfaces/ICargoRepository.h"
#include "../SimulationCore/Interfaces/IDemandService.h"

namespace World {

struct StubRoadGraph : public IRoadGraph {
    bool routeFound;
    StubRoadGraph() : routeFound(true) {}
    virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute) {
        if (!routeFound) return false;
        outRoute.count = 2;
        outRoute.flags[0] = source;
        outRoute.flags[1] = destination;
        return true;
    }
};

struct StubFlagInventory : public IFlagInventory {
    virtual bool ReceiveDelivery(FlagId, ResourceType, uint8_t, uint32_t) { return true; }
};

struct StubCargoRepository : public ICargoRepository {
    virtual void Release(uint32_t) {}
};

struct StubDemandService : public IDemandService {
    int completeCount;
    int onTaskCreatedCount;
    StubDemandService() : completeCount(0), onTaskCreatedCount(0) {}
    virtual void CompleteDemand(uint32_t) { completeCount++; }
    virtual void OnTaskCreated(uint32_t, uint32_t) { onTaskCreatedCount++; }
};

struct CarrierExecFixture {
    StubRoadGraph roads;
    StubFlagInventory inv;
    StubCargoRepository cargo;
    StubDemandService demand;
    TransportController ctrl;

    CarrierExecFixture() : ctrl(roads, inv, cargo, demand) {}

    FlagId AddSourceNode(WorldModel& world, ResourceType type, int amount = 1) {
        int idx = world.transportNodeCount++;
        TransportNode& node = world.transportNodes[idx];
        node.id = (uint8_t)idx;
        node.buffer.Add(type, amount);
        return kNodeDemandFlagBase + node.id;
    }

    FlagId AddDestNode(WorldModel& world) {
        int idx = world.transportNodeCount++;
        TransportNode& node = world.transportNodes[idx];
        node.id = (uint8_t)idx;
        return kNodeDemandFlagBase + node.id;
    }

    void AddRequest(WorldModel& world, ResourceType type, FlagId dest,
                    TransportTaskReason reason = TTR_Production,
                    FlagId origin = kNodeDemandFlagBase)
    {
        if (world.pendingRequestCount >= 128) return;
        TransportRequest& req = world.pendingRequests[world.pendingRequestCount++];
        req.resource = type;
        req.origin = origin;
        req.destination = dest;
        req.reason = reason;
        req.owner = DemandOwner_Production;
        req.fulfilled = false;
        req.demandIndex = kNoDemand;
    }
};

// ─── 1. Pickup ─────────────────────────────────────────────────────────

TEST(Carrier_PickupFromSourceNode) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Wood, 1);
    FlagId dst = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Wood, dst, TTR_Production, src);

    // Before: source has 1, dest has 0
    TransportNode* srcNode = f.ctrl.FindNodeForFlag(world, src);
    TransportNode* dstNode = f.ctrl.FindNodeForFlag(world, dst);
    EXPECT_EQ(srcNode->GetBufferAmount(ResourceType_Wood), 1);
    EXPECT_EQ(dstNode->GetBufferAmount(ResourceType_Wood), 0);

    f.ctrl.Tick(world);

    // After: source drained, dest received
    EXPECT_EQ(srcNode->GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(dstNode->GetBufferAmount(ResourceType_Wood), 1);
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 1);
}

TEST(Carrier_PickupEmptyBuffer) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Wood, 0);  // empty
    FlagId dst = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Wood, dst, TTR_Production, src);

    f.ctrl.Tick(world);

    // Buffer still empty, no delivery happened
    TransportNode* srcNode = f.ctrl.FindNodeForFlag(world, src);
    TransportNode* dstNode = f.ctrl.FindNodeForFlag(world, dst);
    EXPECT_EQ(srcNode->GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(dstNode->GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 0);

    // Carrier stays assigned (pickup failed)
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers - 1);
    EXPECT_EQ(f.ctrl.GetBusyCarrierCount(), 1);
}

TEST(Carrier_PickupRetryOnSubsequentTick) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Wood, 0);  // empty
    FlagId dst = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Wood, dst, TTR_Production, src);

    TransportNode* srcNode = f.ctrl.FindNodeForFlag(world, src);
    TransportNode* dstNode = f.ctrl.FindNodeForFlag(world, dst);

    // First tick — pickup fails
    f.ctrl.Tick(world);
    EXPECT_EQ(dstNode->GetBufferAmount(ResourceType_Wood), 0);

    // Replenish buffer
    srcNode->buffer.Add(ResourceType_Wood, 1);

    // Second tick — pickup succeeds, delivery completes
    f.ctrl.Tick(world);
    EXPECT_EQ(srcNode->GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(dstNode->GetBufferAmount(ResourceType_Wood), 1);
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 1);
}

// ─── 2. Full Lifecycle ─────────────────────────────────────────────────

TEST(Carrier_FullLifecycle) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Planks, 1);
    FlagId dst = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Planks, dst, TTR_Production, src);

    TransportNode* srcNode = f.ctrl.FindNodeForFlag(world, src);
    TransportNode* dstNode = f.ctrl.FindNodeForFlag(world, dst);

    f.ctrl.Tick(world);

    // Carrier completes full lifecycle in one Tick
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers);
    EXPECT_EQ(srcNode->GetBufferAmount(ResourceType_Planks), 0);
    EXPECT_EQ(dstNode->GetBufferAmount(ResourceType_Planks), 1);

    // Delivery event recorded
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 1);
    const TransportController::DeliveryRecord& rec = f.ctrl.GetRecentDelivery(0);
    EXPECT_EQ(rec.resource, ResourceType_Planks);
    EXPECT_EQ(rec.destinationFlag, dst);
    EXPECT_EQ(rec.reason, TTR_Production);
}

// ─── 3. Multiple Carriers ──────────────────────────────────────────────

TEST(Carrier_MultipleConcurrentDeliveries) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src1 = f.AddSourceNode(world, ResourceType_Wood, 1);
    FlagId dst1 = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Wood, dst1, TTR_Production, src1);

    FlagId src2 = f.AddSourceNode(world, ResourceType_Planks, 1);
    FlagId dst2 = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Planks, dst2, TTR_Production, src2);

    TransportNode* dstNode1 = f.ctrl.FindNodeForFlag(world, dst1);
    TransportNode* dstNode2 = f.ctrl.FindNodeForFlag(world, dst2);

    f.ctrl.Tick(world);

    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers);
    EXPECT_EQ(dstNode1->GetBufferAmount(ResourceType_Wood), 1);
    EXPECT_EQ(dstNode2->GetBufferAmount(ResourceType_Planks), 1);

    TransportNode* srcNode1 = f.ctrl.FindNodeForFlag(world, src1);
    TransportNode* srcNode2 = f.ctrl.FindNodeForFlag(world, src2);
    EXPECT_EQ(srcNode1->GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(srcNode2->GetBufferAmount(ResourceType_Planks), 0);
}

TEST(Carrier_ReuseAfterDelivery) {
    CarrierExecFixture f;
    WorldModel world;

    // First delivery
    FlagId src1 = f.AddSourceNode(world, ResourceType_Wood, 1);
    FlagId dst1 = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Wood, dst1, TTR_Production, src1);

    f.ctrl.Tick(world);
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers);

    // Second delivery — same pool reuses carriers
    FlagId src2 = f.AddSourceNode(world, ResourceType_Planks, 1);
    FlagId dst2 = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Planks, dst2, TTR_Production, src2);

    f.ctrl.Tick(world);

    TransportNode* dst1Node = f.ctrl.FindNodeForFlag(world, dst1);
    TransportNode* dst2Node = f.ctrl.FindNodeForFlag(world, dst2);
    EXPECT_EQ(dst1Node->GetBufferAmount(ResourceType_Wood), 1);
    EXPECT_EQ(dst2Node->GetBufferAmount(ResourceType_Planks), 1);
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 2);
}

// ─── 4. Edge Cases ─────────────────────────────────────────────────────

TEST(Carrier_NoSourceNode_StaysAssigned) {
    CarrierExecFixture f;
    WorldModel world;

    // Origin flag has no TransportNode (e.g. real game flag < 400)
    FlagId dst = f.AddDestNode(world);
    f.AddRequest(world, ResourceType_Wood, dst, TTR_Production, 42);

    f.ctrl.Tick(world);

    // Carrier stays assigned (no source node to pick up from)
    EXPECT_EQ(f.ctrl.GetBusyCarrierCount(), 1);
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers - 1);
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 0);
}

TEST(Carrier_PickupRequiresDestNode) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Wood, 1);
    f.AddRequest(world, ResourceType_Wood, 99, TTR_Production, src);

    TransportNode* srcNode = f.ctrl.FindNodeForFlag(world, src);

    f.ctrl.Tick(world);

    // No destination node → pickup skipped, carrier stays assigned
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers - 1);
    EXPECT_EQ(f.ctrl.GetBusyCarrierCount(), 1);
    EXPECT_EQ(f.ctrl.GetRecentDeliveryCount(), 0);
    EXPECT_EQ(srcNode->GetBufferAmount(ResourceType_Wood), 1);  // resource preserved
}

// ─── 5. Multi-hop travel (PR 3.5) ──────────────────────────────────────

TEST(Carrier_MultiHopThreeTicks) {
    CarrierExecFixture f;
    WorldModel world;

    // Multi-hop road graph: 1─2─3─4
    class MultiHopRoadGraph : public World::IRoadGraph {
    public:
        virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute) {
            outRoute.count = 4;
            outRoute.flags[0] = source;
            outRoute.flags[1] = source + 1;
            outRoute.flags[2] = source + 2;
            outRoute.flags[3] = destination;
            return true;
        }
    } multiRoads;

    // Override the fixture's road graph with multi-hop
    StubFlagInventory inv;
    StubCargoRepository cargoRepo;
    StubDemandService demandSvc;
    TransportController ctrl(multiRoads, inv, cargoRepo, demandSvc);

    // Create source + dest transport nodes
    int srcIdx = world.transportNodeCount++;
    TransportNode& srcNode = world.transportNodes[srcIdx];
    srcNode.id = (uint8_t)srcIdx;
    srcNode.buffer.Add(ResourceType_Wood, 1);
    FlagId srcFlag = kNodeDemandFlagBase + srcIdx;

    int dstIdx = world.transportNodeCount++;
    TransportNode& dstNode = world.transportNodes[dstIdx];
    dstNode.id = (uint8_t)dstIdx;
    FlagId dstFlag = kNodeDemandFlagBase + dstIdx;

    // Request: route = [srcFlag, 402, 403, dstFlag] = 3 hops
    if (world.pendingRequestCount < 128) {
        TransportRequest& req = world.pendingRequests[world.pendingRequestCount++];
        req.resource = ResourceType_Wood;
        req.origin = srcFlag;
        req.destination = dstFlag;
        req.reason = TTR_Production;
        req.owner = DemandOwner_Production;
        req.fulfilled = false;
        req.demandIndex = kNoDemand;
    }

    // Tick 1: Assigned → Pickup → Travelling (hop 0→1, still travelling)
    ctrl.Tick(world);
    EXPECT_EQ(ctrl.GetIdleCarrierCount(), kMaxCarriers - 1);
    EXPECT_EQ(ctrl.GetBusyCarrierCount(), 1);
    EXPECT_EQ(srcNode.GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(dstNode.GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 0);

    // Tick 2: Travelling → Travelling (hop 1→2, still travelling)
    ctrl.Tick(world);
    EXPECT_EQ(ctrl.GetIdleCarrierCount(), kMaxCarriers - 1);
    EXPECT_EQ(ctrl.GetBusyCarrierCount(), 1);
    EXPECT_EQ(dstNode.GetBufferAmount(ResourceType_Wood), 0);
    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 0);

    // Tick 3: Travelling → Delivering → Idle
    ctrl.Tick(world);
    EXPECT_EQ(ctrl.GetIdleCarrierCount(), kMaxCarriers);
    EXPECT_EQ(ctrl.GetBusyCarrierCount(), 0);
    EXPECT_EQ(dstNode.GetBufferAmount(ResourceType_Wood), 1);
    EXPECT_EQ(ctrl.GetRecentDeliveryCount(), 1);
}

// ─── 6. Demand completion via Tick() path (PR 3.6) ────────────────────

TEST(Carrier_TickDeliveryCallsCompleteDemand) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Wood, 1);
    FlagId dst = f.AddDestNode(world);

    // Add request with a valid demandIndex so observerTicketId is set
    if (world.pendingRequestCount < 128) {
        TransportRequest& req = world.pendingRequests[world.pendingRequestCount++];
        req.resource = ResourceType_Wood;
        req.origin = src;
        req.destination = dst;
        req.reason = TTR_Production;
        req.owner = DemandOwner_Production;
        req.fulfilled = false;
        req.demandIndex = 0;  // non-kNoDemand → observerTicketId = 1
    }

    f.ctrl.Tick(world);

    // Carrier completed delivery → CompleteDemand was called once
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers);
    EXPECT_EQ(f.demand.completeCount, 1);
}

TEST(Carrier_NoDemandIndex_SkipsCompleteDemand) {
    CarrierExecFixture f;
    WorldModel world;

    FlagId src = f.AddSourceNode(world, ResourceType_Wood, 1);
    FlagId dst = f.AddDestNode(world);

    f.AddRequest(world, ResourceType_Wood, dst, TTR_Production, src);

    f.ctrl.Tick(world);

    // No demandIndex → observerTicketId = 0 → CompleteDemand not called
    EXPECT_EQ(f.ctrl.GetIdleCarrierCount(), kMaxCarriers);
    EXPECT_EQ(f.demand.completeCount, 0);
}

// ─── 7. Conservation invariant ─────────────────────────────────────────

TEST(Carrier_ResourceConservation) {
    CarrierExecFixture f;
    WorldModel world;

    // Setup: two source nodes and two destination nodes
    FlagId src1 = f.AddSourceNode(world, ResourceType_Wood, 3);
    FlagId src2 = f.AddSourceNode(world, ResourceType_Planks, 2);
    FlagId dst1 = f.AddDestNode(world);
    FlagId dst2 = f.AddDestNode(world);

    // Total resources at start
    TransportNode* srcNode1 = f.ctrl.FindNodeForFlag(world, src1);
    TransportNode* srcNode2 = f.ctrl.FindNodeForFlag(world, src2);
    TransportNode* dstNode1 = f.ctrl.FindNodeForFlag(world, dst1);
    TransportNode* dstNode2 = f.ctrl.FindNodeForFlag(world, dst2);

    int totalBefore = srcNode1->GetBufferAmount(ResourceType_Wood)
                   + srcNode2->GetBufferAmount(ResourceType_Planks)
                   + dstNode1->GetBufferAmount(ResourceType_Wood)
                   + dstNode2->GetBufferAmount(ResourceType_Planks);

    // Create two transport requests
    f.AddRequest(world, ResourceType_Wood, dst1, TTR_Production, src1);
    f.AddRequest(world, ResourceType_Planks, dst2, TTR_Production, src2);

    f.ctrl.Tick(world);

    // Sum: node buffers + carrier cargo
    int totalNode = srcNode1->GetBufferAmount(ResourceType_Wood)
                  + srcNode2->GetBufferAmount(ResourceType_Planks)
                  + dstNode1->GetBufferAmount(ResourceType_Wood)
                  + dstNode2->GetBufferAmount(ResourceType_Planks);

    int totalCargo = 0;
    for (int i = 0; i < kMaxCarriers; ++i) {
        if (f.ctrl.GetCarrierPool()[i].cargoType != ResourceType_None) {
            totalCargo += f.ctrl.GetCarrierPool()[i].cargoAmount;
        }
    }

    // Total should be conserved: 3 Wood + 2 Planks = 5 units
    // After delivery: Wood moved from src1 → dst1, Planks from src2 → dst2
    EXPECT_EQ(totalNode + totalCargo, totalBefore);

    // Verify the actual distribution
    EXPECT_EQ(srcNode1->GetBufferAmount(ResourceType_Wood), 2);   // 3-1 = 2
    EXPECT_EQ(dstNode1->GetBufferAmount(ResourceType_Wood), 1);   // 0+1 = 1
    EXPECT_EQ(srcNode2->GetBufferAmount(ResourceType_Planks), 1); // 2-1 = 1
    EXPECT_EQ(dstNode2->GetBufferAmount(ResourceType_Planks), 1); // 0+1 = 1
    EXPECT_EQ(totalCargo, 0);  // all carriers idle, cargo cleared
}

} // namespace World
