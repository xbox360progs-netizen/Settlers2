#include "TestRunner.h"
#include "../SimulationCore/Transport/TransportNode.h"

TEST(TransportNode_InitiallyEmpty) {
    World::TransportNode node;
    EXPECT_EQ(node.attachmentCount, 0);
    EXPECT_EQ(node.buffer.slotCount, 0);
    EXPECT_EQ(node.outgoingCount, 0);
}

TEST(TransportNode_AttachBuilding) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    EXPECT_EQ(node.attachmentCount, 1);
    EXPECT_EQ(node.attachments[0].buildingId, 1);
    EXPECT_EQ(node.attachments[0].role, World::AR_Consumer);
    EXPECT_EQ(node.attachments[0].inputCount, 1);
    EXPECT_EQ(node.attachments[0].inputs[0], World::ResourceType_Wood);
}

TEST(TransportNode_AttachProducer) {
    World::TransportNode node;
    node.AttachBuilding(2, NULL, 0, World::AR_Producer);
    EXPECT_EQ(node.attachmentCount, 1);
    EXPECT_EQ(node.attachments[0].role, World::AR_Producer);
    EXPECT_EQ(node.attachments[0].inputCount, 0);
}

TEST(TransportNode_AttachDuplicate) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    EXPECT_EQ(node.attachmentCount, 1);
}

TEST(TransportNode_DetachBuilding) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    node.AttachBuilding(2, inputs, 1, World::AR_Consumer);
    EXPECT_EQ(node.attachmentCount, 2);

    node.DetachBuilding(1);
    EXPECT_EQ(node.attachmentCount, 1);
    EXPECT_EQ(node.attachments[0].buildingId, 2);
}

TEST(TransportNode_DetachNotAttached) {
    World::TransportNode node;
    node.DetachBuilding(99);
    EXPECT_EQ(node.attachmentCount, 0);
}

TEST(TransportNode_ReceiveCargo) {
    World::TransportNode node;
    node.ReceiveCargo(World::ResourceType_Wood, 5);
    EXPECT_EQ(node.buffer.Count(World::ResourceType_Wood), 5);
}

TEST(TransportNode_ReceiveExport) {
    World::TransportNode node;
    node.ReceiveExport(World::ResourceType_Planks, 3);
    EXPECT_EQ(node.buffer.Count(World::ResourceType_Planks), 3);
}

TEST(TransportNode_TakeForBuilding_Success) {
    World::TransportNode node;
    node.ReceiveCargo(World::ResourceType_Wood, 5);
    bool taken = node.TakeForBuilding(1, World::ResourceType_Wood, 2);
    EXPECT_TRUE(taken);
    EXPECT_EQ(node.buffer.Count(World::ResourceType_Wood), 3);
}

TEST(TransportNode_TakeForBuilding_Fail) {
    World::TransportNode node;
    node.ReceiveCargo(World::ResourceType_Wood, 1);
    bool taken = node.TakeForBuilding(1, World::ResourceType_Wood, 2);
    EXPECT_FALSE(taken);
    EXPECT_EQ(node.buffer.Count(World::ResourceType_Wood), 1);
}

TEST(TransportNode_TakeForBuilding_Empty) {
    World::TransportNode node;
    bool taken = node.TakeForBuilding(1, World::ResourceType_Wood, 1);
    EXPECT_FALSE(taken);
}

TEST(TransportNode_GetBufferAmount) {
    World::TransportNode node;
    node.ReceiveCargo(World::ResourceType_Wood, 7);
    EXPECT_EQ(node.GetBufferAmount(World::ResourceType_Wood), 7);
    EXPECT_EQ(node.GetBufferAmount(World::ResourceType_Stone), 0);
}

TEST(TransportNode_HasCapacity) {
    World::TransportNode node;
    // Empty buffer has capacity
    EXPECT_TRUE(node.HasCapacity(World::ResourceType_Wood, 1));
    EXPECT_TRUE(node.HasCapacity(World::ResourceType_Wood, World::kNodeBufferSlots));
    // Fill to near max
    // Use types that don't collide with ResourceType_Wood (value 1)
    for (int i = 0; i < World::kNodeBufferSlots; ++i) {
        node.ReceiveCargo(static_cast<World::ResourceType>(static_cast<int>(World::ResourceType_Wood) + i + 1), 1);
    }
    EXPECT_FALSE(node.HasCapacity(World::ResourceType_Wood, 1));
}

TEST(TransportNode_HasDemandFor_NoAttachments) {
    World::TransportNode node;
    EXPECT_FALSE(node.HasDemandFor(World::ResourceType_Wood));
}

TEST(TransportNode_HasDemandFor_ConsumerNeedsResource) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    // Buffer empty, consumer needs wood
    EXPECT_TRUE(node.HasDemandFor(World::ResourceType_Wood));
}

TEST(TransportNode_HasDemandFor_ConsumerSatisfied) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    node.ReceiveCargo(World::ResourceType_Wood, 1);
    // Buffer has wood, consumer satisfied
    EXPECT_FALSE(node.HasDemandFor(World::ResourceType_Wood));
}

TEST(TransportNode_HasDemandFor_ProducerDoesNotTriggerDemand) {
    World::TransportNode node;
    node.AttachBuilding(1, NULL, 0, World::AR_Producer);
    EXPECT_FALSE(node.HasDemandFor(World::ResourceType_Wood));
}

TEST(TransportNode_Tick_NoAttachments) {
    World::TransportNode node;
    node.ReceiveCargo(World::ResourceType_Wood, 5);
    node.Tick();
    // No attachments → no demand, but outgoing should count surplus
    bool hasDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active) hasDemand = true;
    }
    EXPECT_FALSE(hasDemand);
    EXPECT_EQ(node.outgoingCount, 5);
}

TEST(TransportNode_Tick_CreatesDemand) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    // Buffer empty → demand for wood
    node.Tick();
    bool foundDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active && node.pendingDemand[i].resource == World::ResourceType_Wood) {
            foundDemand = true;
            EXPECT_EQ(node.pendingDemand[i].amount, 1);
        }
    }
    EXPECT_TRUE(foundDemand);
    // No surplus — buffer empty
    EXPECT_EQ(node.outgoingCount, 0);
}

TEST(TransportNode_Tick_DemandSatisfiedAfterReceive) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    node.Tick();
    // Demand should exist
    bool preDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active) preDemand = true;
    }
    EXPECT_TRUE(preDemand);

    // Deliver cargo
    node.ReceiveCargo(World::ResourceType_Wood, 2);
    node.Tick();
    // Demand should be gone
    bool postDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active) postDemand = true;
    }
    EXPECT_FALSE(postDemand);
    // Buffer surplus = 2 (no local consumer needs it anymore since buffer >= 1)
    EXPECT_EQ(node.outgoingCount, 2);
}

TEST(TransportNode_Tick_TakeForBuildingThenDemand) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_Consumer);
    node.ReceiveCargo(World::ResourceType_Wood, 1);
    node.Tick();
    // Demand satisfied initially
    bool preDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active) preDemand = true;
    }
    EXPECT_FALSE(preDemand);

    // LocalTransferSystem takes the wood
    node.TakeForBuilding(1, World::ResourceType_Wood, 1);
    node.Tick();
    // Demand should reappear
    bool postDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active) postDemand = true;
    }
    EXPECT_TRUE(postDemand);
}

TEST(TransportNode_Tick_ProducerDoesNotTriggerDemand) {
    World::TransportNode node;
    node.AttachBuilding(1, NULL, 0, World::AR_Producer);
    node.Tick();
    bool hasDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active) hasDemand = true;
    }
    EXPECT_FALSE(hasDemand);
}

TEST(TransportNode_Tick_ProducerConsumerDemand) {
    World::TransportNode node;
    World::ResourceType inputs[] = { World::ResourceType_Wood };
    node.AttachBuilding(1, inputs, 1, World::AR_ProducerConsumer);
    node.Tick();
    // Should create demand for wood
    bool foundDemand = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (node.pendingDemand[i].active && node.pendingDemand[i].resource == World::ResourceType_Wood) {
            foundDemand = true;
        }
    }
    EXPECT_TRUE(foundDemand);
}

TEST(TransportNode_MultipleAttachments) {
    World::TransportNode node;
    World::ResourceType woodInput[] = { World::ResourceType_Wood };
    World::ResourceType stoneInput[] = { World::ResourceType_Stone };
    node.AttachBuilding(1, woodInput, 1, World::AR_Consumer);
    node.AttachBuilding(2, stoneInput, 1, World::AR_Consumer);
    node.Tick();
    // Should have demand for both wood and stone
    bool foundWood = false, foundStone = false;
    for (int i = 0; i < World::kMaxNodeDemands; ++i) {
        if (!node.pendingDemand[i].active) continue;
        if (node.pendingDemand[i].resource == World::ResourceType_Wood) foundWood = true;
        if (node.pendingDemand[i].resource == World::ResourceType_Stone) foundStone = true;
    }
    EXPECT_TRUE(foundWood);
    EXPECT_TRUE(foundStone);
}
