#pragma once
#include <stdint.h>

namespace World {

    struct SimulationState {
        uint32_t tickCount;
        int activeTransportTasks;
        int blockedTransportTasks;
        bool worldLoaded;

        SimulationState()
            : tickCount(0)
            , activeTransportTasks(0)
            , blockedTransportTasks(0)
            , worldLoaded(false)
        {
        }
    };

} // namespace World
