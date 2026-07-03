#pragma once
#include <cstdio>

#define TEST(name) \
    static void Test_##name(); \
    namespace { struct TReg_##name { TReg_##name() { ::TestRunner::Register(#name, Test_##name); } } s_treg_##name; } \
    static void Test_##name()

#define EXPECT_TRUE(expr) do { \
    ::TestRunner::s_passed++; \
    if (!(expr)) { \
        ::TestRunner::s_failed++; \
        std::printf("  %s:%d: EXPECT_TRUE(%s)\n", __FILE__, __LINE__, #expr); \
    } \
} while(0)

#define EXPECT_FALSE(expr) do { \
    ::TestRunner::s_passed++; \
    if ((expr)) { \
        ::TestRunner::s_failed++; \
        std::printf("  %s:%d: EXPECT_FALSE(%s)\n", __FILE__, __LINE__, #expr); \
    } \
} while(0)

#define EXPECT_EQ(a, b) do { \
    ::TestRunner::s_passed++; \
    if ((a) != (b)) { \
        ::TestRunner::s_failed++; \
        std::printf("  %s:%d: EXPECT_EQ(%s, %s) left=%d right=%d\n", __FILE__, __LINE__, #a, #b, (int)(a), (int)(b)); \
    } \
} while(0)

namespace TestRunner {
    void Register(const char* name, void (*fn)());
    int RunAll();
    extern int s_passed;
    extern int s_failed;
}
