#include "TestRunner.h"
#include "../SimulationCore/Transport/Cargo.h"
#include "../SimulationCore/Core/ResourceTypes.h"

TEST(Cargo_DefaultConstruction) {
    World::Cargo c;
    EXPECT_EQ(c.id, 0u);
    EXPECT_EQ(c.type, World::ResourceType_None);
    EXPECT_EQ(c.amount, 0u);
    EXPECT_EQ(c.state, World::Cargo_OnFlag);
    EXPECT_TRUE(c.ownerTask == NULL);
    EXPECT_FALSE(c.currentFlag.IsValid());
}

TEST(Cargo_SetFields) {
    World::Cargo c;
    c.id = 42;
    c.type = World::ResourceType_Wood;
    c.amount = 3;
    c.state = World::Cargo_Carried;
    EXPECT_EQ(c.id, 42u);
    EXPECT_EQ(c.type, World::ResourceType_Wood);
    EXPECT_EQ(c.amount, 3u);
    EXPECT_EQ(c.state, World::Cargo_Carried);
}
