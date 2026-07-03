#include "TestRunner.h"

int main() {
    std::printf("SimulationCore Tests\n");
    std::printf("====================\n\n");
    int failures = TestRunner::RunAll();
    return failures > 0 ? 1 : 0;
}
