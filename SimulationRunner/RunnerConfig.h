#pragma once
#include <stdint.h>

struct RunnerConfig {
    uint32_t tickCount;
    uint32_t telemetryInterval;

    RunnerConfig()
        : tickCount(10000)
        , telemetryInterval(1000)
    {
    }
};
