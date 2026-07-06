#include "TestRunner.h"
#include "../Platform/Lock.h"
#include "../Platform/Atomic.h"
#include "../Platform/Timer.h"
#include "../Platform/Event.h"

TEST(Platform_Lock_AcquireRelease) {
    Platform::Lock lock;
    lock.Acquire();
    lock.Release();
}

TEST(Platform_Atomic_Increment) {
    volatile long val = 0;
    long result = Platform::AtomicIncrement(&val);
    EXPECT_EQ(result, 1L);
    EXPECT_EQ(val, 1L);
}

TEST(Platform_Atomic_Decrement) {
    volatile long val = 5;
    long result = Platform::AtomicDecrement(&val);
    EXPECT_EQ(result, 4L);
    EXPECT_EQ(val, 4L);
}

TEST(Platform_Atomic_Exchange) {
    volatile long val = 10;
    long old = Platform::AtomicExchange(&val, 20);
    EXPECT_EQ(old, 10L);
    EXPECT_EQ(val, 20L);
}

TEST(Platform_Atomic_CompareExchange) {
    volatile long val = 30;
    // Successful CAS
    long old = Platform::AtomicCompareExchange(&val, 40, 30);
    EXPECT_EQ(old, 30L);
    EXPECT_EQ(val, 40L);

    // Failed CAS
    val = 50;
    old = Platform::AtomicCompareExchange(&val, 99, 30);
    EXPECT_EQ(old, 50L);
    EXPECT_EQ(val, 50L);
}

TEST(Platform_Atomic_MemoryFence) {
    volatile long a = 1;
    volatile long b = 2;
    a = 3;
    Platform::MemoryFence();
    b = a;
    EXPECT_EQ(b, 3L);
}

TEST(Platform_Timer_GetTickCount) {
    unsigned int t1 = Platform::GetTickCount();
    Platform::Sleep(10);
    unsigned int t2 = Platform::GetTickCount();
    EXPECT_TRUE(t2 >= t1);
}

TEST(Platform_Timer_Sleep) {
    unsigned int t1 = Platform::GetTickCount();
    Platform::Sleep(15);
    unsigned int t2 = Platform::GetTickCount();
    unsigned int diff = t2 - t1;
    EXPECT_TRUE(diff >= 10);
}

TEST(Platform_Event_AutoReset) {
    Platform::Event ev(false);
    ev.Signal();
    bool ok = ev.Wait(0);
    EXPECT_TRUE(ok);

    // After wait, auto-reset event should be reset
    ok = ev.Wait(0);
    EXPECT_FALSE(ok);
}

TEST(Platform_Event_ManualReset) {
    Platform::Event ev(true);
    ev.Signal();
    bool ok = ev.Wait(0);
    EXPECT_TRUE(ok);

    // Manual-reset stays signalled
    ok = ev.Wait(0);
    EXPECT_TRUE(ok);

    ev.Reset();
    ok = ev.Wait(0);
    EXPECT_FALSE(ok);
}

TEST(Platform_Event_Timeout) {
    Platform::Event ev(false);
    bool ok = ev.Wait(10);
    EXPECT_FALSE(ok);
}
