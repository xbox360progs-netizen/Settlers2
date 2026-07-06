#include "TestRunner.h"
#include "../SimulationCore/Transport/LocalTransferSystem.h"
#include "../SimulationCore/World/WorldModel.h"

// ─── Export: Building -> Node ──────────────────────────────────────────

TEST(LocalTransfer_ProducerExportsToEmptyBuffer) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 5);
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
}

TEST(LocalTransfer_ProducerExportsToPartialBuffer) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 3;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 2);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 5);
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
}

TEST(LocalTransfer_ProducerBlockedByFullBuffer) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 3;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);
    // Fill buffer with 8 different resources
    for (int i = 0; i < World::kNodeBufferSlots; ++i) {
        world.transportNodes[0].ReceiveCargo(
            static_cast<World::ResourceType>(static_cast<int>(World::ResourceType_Wood) + i + 1), 1);
    }

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Buffer full, no space for Wood
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 0);
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 3);
}

TEST(LocalTransfer_ProducerExportsMultipleTypes) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 2;
    world.productionBuildings[0].outputResources[1] = World::ResourceType_Planks;
    world.productionBuildings[0].outputBuffer[1] = 3;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 2);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Planks), 3);
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[1], 0);
}

// ─── Supply: Node -> Building ──────────────────────────────────────────

TEST(LocalTransfer_ConsumerTakesFromBuffer) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 3;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_Consumer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 5);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    EXPECT_EQ(world.productionBuildings[0].inputDelivered[0], 3);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 2);
}

TEST(LocalTransfer_ConsumerBlockedByEmptyBuffer) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 3;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_Consumer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    EXPECT_EQ(world.productionBuildings[0].inputDelivered[0], 0);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 0);
}

TEST(LocalTransfer_ConsumerPartialSupply) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 5;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_Consumer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 3);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    EXPECT_EQ(world.productionBuildings[0].inputDelivered[0], 3);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 0);
}

// ─── ProducerConsumer ──────────────────────────────────────────────────

TEST(LocalTransfer_ProducerConsumerExportsAndSupplies) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    // Sawmill: consumes Wood, produces Planks
    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 2;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Planks;
    world.productionBuildings[0].outputBuffer[0] = 2;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_ProducerConsumer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 3);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Exported 2 Planks, supplied 2 Wood
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Planks), 2);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 1);
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
    EXPECT_EQ(world.productionBuildings[0].inputDelivered[0], 2);
}

// ─── Multiple Attachments ──────────────────────────────────────────────

TEST(LocalTransfer_MultipleAttachmentsMixedRoles) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 2;

    // Building 0: Producer (Wood)
    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 3;

    // Building 1: Consumer (needs Stone)
    world.productionBuildings[1].active = true;
    world.productionBuildings[1].inputResources[0] = World::ResourceType_Stone;
    world.productionBuildings[1].inputRequired[0] = 2;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);
    World::ResourceType stoneInput[] = { World::ResourceType_Stone };
    world.transportNodes[0].AttachBuilding(1, stoneInput, 1, World::AR_Consumer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Stone, 3);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Producer exported 3 Wood, Consumer took 2 Stone
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 3);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Stone), 1);
    EXPECT_EQ(world.productionBuildings[1].inputDelivered[0], 2);
}

TEST(LocalTransfer_ProducerSuppliesConsumerOnSameNode) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 2;

    // Building 0: Producer (output 5 Wood)
    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    // Building 1: Consumer (needs 3 Wood)
    world.productionBuildings[1].active = true;
    world.productionBuildings[1].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[1].inputRequired[0] = 3;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);
    World::ResourceType woodInput[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(1, woodInput, 1, World::AR_Consumer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Producer exported 5 Wood. Consumer took 3 from buffer.
    // Buffer has 2 Wood remaining.
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
    EXPECT_EQ(world.productionBuildings[1].inputDelivered[0], 3);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 2);
}

// ─── Edge Cases ────────────────────────────────────────────────────────

TEST(LocalTransfer_NoAttachments) {
    World::WorldModel world;
    world.transportNodeCount = 0;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Nothing happens — no nodes
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 5);
}

TEST(LocalTransfer_InactiveBuilding) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = false;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Inactive building, no transfer
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 5);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 0);
}

TEST(LocalTransfer_BuildingIdOutOfRange) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    // Attach buildingId=5 which is >= productionBuildingCount (1)
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(5, inputs, 1, World::AR_Consumer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Out-of-range building skipped
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 5);
}

// ─── Tick Evaluation ───────────────────────────────────────────────────

TEST(LocalTransfer_TickEvaluationNoDeficit) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 2;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Planks;
    world.productionBuildings[0].outputBuffer[0] = 2;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_ProducerConsumer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 2);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // After transfer: Wood deficit satisfied (2 delivered), Planks exported (2)
    // No active deficit — pendingDemand should be empty
    bool hasDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (world.transportNodes[0].pendingDemand[i].active) {
            hasDemand = true;
        }
    }
    EXPECT_FALSE(hasDemand);
    // outgoingCount = 2 Planks (no local consumer)
    EXPECT_EQ(world.transportNodes[0].outgoingCount, 2);
}

TEST(LocalTransfer_TickEvaluationStillDeficit) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 3;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_Consumer);
    // Buffer has only 2 Wood, building needs 3
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 2);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // After transfer: 2 delivered, still needs 1
    EXPECT_EQ(world.productionBuildings[0].inputDelivered[0], 2);

    // pendingDemand should have Wood (deficit = 1)
    bool foundDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (world.transportNodes[0].pendingDemand[i].active &&
            world.transportNodes[0].pendingDemand[i].resource == World::ResourceType_Wood) {
            foundDemand = true;
        }
    }
    EXPECT_TRUE(foundDemand);
    // outgoingCount = 0 (buffer empty)
    EXPECT_EQ(world.transportNodes[0].outgoingCount, 0);
}

TEST(LocalTransfer_TickEvaluationSurplus) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].inputRequired[0] = 2;

    World::ResourceType inputs[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(0, inputs, 1, World::AR_Consumer);
    world.transportNodes[0].ReceiveCargo(World::ResourceType_Wood, 5);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // 2 delivered to building, 3 remain in buffer
    EXPECT_EQ(world.productionBuildings[0].inputDelivered[0], 2);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 3);

    // No deficit — demand satisfied
    bool hasDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (world.transportNodes[0].pendingDemand[i].active) hasDemand = true;
    }
    EXPECT_FALSE(hasDemand);
    // outgoingCount = 3 (surplus)
    EXPECT_EQ(world.transportNodes[0].outgoingCount, 3);
}

// ─── Idempotency ───────────────────────────────────────────────────────

TEST(LocalTransfer_IdempotentTick) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 2;

    // Building 0: Producer (output 5 Wood)
    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    // Building 1: Consumer (needs 3 Wood)
    world.productionBuildings[1].active = true;
    world.productionBuildings[1].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[1].inputRequired[0] = 3;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);
    World::ResourceType woodInput[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(1, woodInput, 1, World::AR_Consumer);

    World::LocalTransferSystem sys;
    sys.Tick(world);
    sys.Tick(world);

    // Second Tick changes nothing: no new output, no unsatisfied demand
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
    EXPECT_EQ(world.productionBuildings[1].inputDelivered[0], 3);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 2);
    EXPECT_EQ(world.transportNodes[0].outgoingCount, 2);
}

// ─── Full Cycle ────────────────────────────────────────────────────────

TEST(LocalTransfer_FullCycleProducerToConsumer) {
    World::WorldModel world;
    world.transportNodeCount = 1;
    world.productionBuildingCount = 2;

    // Producer: outputs Wood
    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    // Consumer: needs Wood
    world.productionBuildings[1].active = true;
    world.productionBuildings[1].inputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[1].inputRequired[0] = 3;

    world.transportNodes[0].AttachBuilding(0, NULL, 0, World::AR_Producer);
    World::ResourceType woodInput[] = { World::ResourceType_Wood };
    world.transportNodes[0].AttachBuilding(1, woodInput, 1, World::AR_Consumer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Full cycle verified: Producer -> Node -> Consumer
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 0);
    EXPECT_EQ(world.productionBuildings[1].inputDelivered[0], 3);
    EXPECT_EQ(world.transportNodes[0].GetBufferAmount(World::ResourceType_Wood), 2);
}

TEST(LocalTransfer_NodeCountRespected) {
    // Only transfers within the first transportNodeCount nodes
    World::WorldModel world;
    world.transportNodeCount = 1;  // Only first node is active
    world.productionBuildingCount = 1;

    world.productionBuildings[0].active = true;
    world.productionBuildings[0].outputResources[0] = World::ResourceType_Wood;
    world.productionBuildings[0].outputBuffer[0] = 5;

    // Attach to second node (index 1) which is beyond transportNodeCount
    world.transportNodes[1].AttachBuilding(0, NULL, 0, World::AR_Producer);

    World::LocalTransferSystem sys;
    sys.Tick(world);

    // Second node is not iterated because transportNodeCount=1
    EXPECT_EQ(world.productionBuildings[0].outputBuffer[0], 5);
    EXPECT_EQ(world.transportNodes[1].GetBufferAmount(World::ResourceType_Wood), 0);
}
