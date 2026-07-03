#include "TestRunner.h"

namespace TestRunner {

    struct TestEntry {
        const char* name;
        void (*fn)();
        TestEntry* next;
        static TestEntry* s_head;
    };

    TestEntry* TestEntry::s_head = NULL;
    int s_passed = 0;
    int s_failed = 0;

    void Register(const char* name, void (*fn)()) {
        TestEntry* e = new TestEntry();
        e->name = name;
        e->fn = fn;
        e->next = TestEntry::s_head;
        TestEntry::s_head = e;
    }

    int RunAll() {
        int totalFail = 0, totalTests = 0;
        for (TestEntry* t = TestEntry::s_head; t; t = t->next) {
            s_passed = 0;
            s_failed = 0;
            t->fn();
            totalTests++;
            if (s_failed == 0)
                std::printf("  PASS  %s\n", t->name);
            else
                std::printf("  FAIL  %s (%d/%d)\n", t->name, s_passed, s_passed + s_failed);
            totalFail += s_failed;
        }
        std::printf("\n%d tests, %d failures\n", totalTests, totalFail);
        return totalFail;
    }

} // namespace TestRunner
