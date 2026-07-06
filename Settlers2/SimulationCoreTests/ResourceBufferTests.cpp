#include "TestRunner.h"
#include "../SimulationCore/Transport/ResourceBuffer.h"

TEST(ResourceBuffer_InitiallyEmpty) {
    World::ResourceBuffer buf;
    EXPECT_EQ(buf.slotCount, 0);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 0);
}

TEST(ResourceBuffer_AddSingle) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 3);
    EXPECT_EQ(buf.slotCount, 1);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 3);
}

TEST(ResourceBuffer_AddMultipleTypes) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 3);
    buf.Add(World::ResourceType_Stone, 2);
    EXPECT_EQ(buf.slotCount, 2);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 3);
    EXPECT_EQ(buf.Count(World::ResourceType_Stone), 2);
}

TEST(ResourceBuffer_AddToExisting) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 3);
    buf.Add(World::ResourceType_Wood, 2);
    EXPECT_EQ(buf.slotCount, 1);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 5);
}

TEST(ResourceBuffer_RemoveExact) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 5);
    int removed = buf.Remove(World::ResourceType_Wood, 3);
    EXPECT_EQ(removed, 3);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 2);
}

TEST(ResourceBuffer_RemoveAll) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 5);
    int removed = buf.Remove(World::ResourceType_Wood, 5);
    EXPECT_EQ(removed, 5);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 0);
    // Slot should be freed
    EXPECT_EQ(buf.slotCount, 0);
}

TEST(ResourceBuffer_RemoveMoreThanAvailable) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 2);
    int removed = buf.Remove(World::ResourceType_Wood, 10);
    EXPECT_EQ(removed, 2);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 0);
}

TEST(ResourceBuffer_RemoveFromEmpty) {
    World::ResourceBuffer buf;
    int removed = buf.Remove(World::ResourceType_Wood, 1);
    EXPECT_EQ(removed, 0);
}

TEST(ResourceBuffer_HasExact) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 5);
    EXPECT_TRUE(buf.Has(World::ResourceType_Wood, 3));
    EXPECT_TRUE(buf.Has(World::ResourceType_Wood, 5));
    EXPECT_FALSE(buf.Has(World::ResourceType_Wood, 6));
}

TEST(ResourceBuffer_HasEmpty) {
    World::ResourceBuffer buf;
    EXPECT_FALSE(buf.Has(World::ResourceType_Wood, 1));
}

TEST(ResourceBuffer_BufferFull) {
    World::ResourceBuffer buf;
    // Fill all slots with different resources (avoid ResourceType_Wood = 1 collision)
    for (int i = 0; i < World::kNodeBufferSlots; ++i) {
        buf.Add(static_cast<World::ResourceType>(static_cast<int>(World::ResourceType_Wood) + i + 1), 1);
    }
    EXPECT_EQ(buf.slotCount, World::kNodeBufferSlots);
    // Next add should be dropped
    buf.Add(World::ResourceType_Wood, 1);
    EXPECT_EQ(buf.slotCount, World::kNodeBufferSlots);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 0);
}

TEST(ResourceBuffer_RemoveNoneType) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 5);
    int removed = buf.Remove(World::ResourceType_None, 1);
    EXPECT_EQ(removed, 0);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 5);
}

TEST(ResourceBuffer_AddZeroAmount) {
    World::ResourceBuffer buf;
    buf.Add(World::ResourceType_Wood, 0);
    EXPECT_EQ(buf.slotCount, 0);
    EXPECT_EQ(buf.Count(World::ResourceType_Wood), 0);
}
