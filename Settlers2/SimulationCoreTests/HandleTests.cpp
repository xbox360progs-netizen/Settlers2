#include "TestRunner.h"
#include "../SimulationCore/Core/Handle.h"

TEST(Handle_DefaultIsInvalid) {
    World::Handle<int> h;
    EXPECT_FALSE(h.IsValid());
}

TEST(Handle_DefaultIndexIsInvalid) {
    World::Handle<int> h;
    EXPECT_EQ(h.index, 0xFFFFFFFFu);
}

TEST(Handle_DefaultGenerationIsZero) {
    World::Handle<int> h;
    EXPECT_EQ(h.generation, 0u);
}

TEST(Handle_ExplicitConstruction) {
    World::Handle<int> h(5, 1);
    EXPECT_TRUE(h.IsValid());
    EXPECT_EQ(h.index, 5u);
    EXPECT_EQ(h.generation, 1u);
}

TEST(Handle_EqualitySame) {
    World::Handle<int> a(5, 1);
    World::Handle<int> b(5, 1);
    EXPECT_TRUE(a == b);
}

TEST(Handle_EqualityDifferentIndex) {
    World::Handle<int> a(5, 1);
    World::Handle<int> b(6, 1);
    EXPECT_TRUE(a != b);
}

TEST(Handle_EqualityDifferentGeneration) {
    World::Handle<int> a(5, 1);
    World::Handle<int> b(5, 2);
    EXPECT_TRUE(a != b);
}

TEST(Handle_InvalidNotEqual) {
    World::Handle<int> a;
    World::Handle<int> b(0, 0);
    EXPECT_TRUE(a != b);
}

TEST(Handle_RegistryRegisterResolve) {
    World::HandleRegistry reg;
    int value = 42;
    World::Handle<int> h = reg.Register(&value);
    EXPECT_TRUE(h.IsValid());
    int* resolved = reg.Resolve(h);
    EXPECT_TRUE(resolved != NULL);
    EXPECT_EQ(*resolved, 42);
}

TEST(Handle_RegistryUnregisterInvalidates) {
    World::HandleRegistry reg;
    int value = 42;
    World::Handle<int> h = reg.Register(&value);
    reg.Unregister(h);
    EXPECT_TRUE(reg.Resolve(h) == NULL);
}

TEST(Handle_RegistryGenerationIncrement) {
    World::HandleRegistry reg;
    int a = 1, b = 2;
    World::Handle<int> h1 = reg.Register(&a);
    reg.Unregister(h1);
    World::Handle<int> h2 = reg.Register(&b);
    EXPECT_TRUE(h2.IsValid());
    EXPECT_EQ(h2.index, h1.index);
    EXPECT_TRUE(h2.generation != h1.generation);
    EXPECT_TRUE(reg.Resolve(h1) == NULL);
    EXPECT_TRUE(reg.Resolve(h2) != NULL);
    EXPECT_EQ(*reg.Resolve(h2), 2);
}

TEST(Handle_RegistryClear) {
    World::HandleRegistry reg;
    int value = 42;
    World::Handle<int> h = reg.Register(&value);
    reg.Clear();
    EXPECT_TRUE(reg.Resolve(h) == NULL);
}

TEST(Handle_RegistryFindHandle) {
    World::HandleRegistry reg;
    int value = 42;
    World::Handle<int> h = reg.Register(&value);
    World::Handle<int> found = reg.FindHandle(&value);
    EXPECT_TRUE(found == h);
}

TEST(Handle_RegistryFindHandleNotFound) {
    World::HandleRegistry reg;
    int value = 42;
    World::Handle<int> found = reg.FindHandle(&value);
    EXPECT_FALSE(found.IsValid());
}
