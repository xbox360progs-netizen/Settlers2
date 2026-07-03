#pragma once
#include <stdint.h>

namespace World {

    struct SimulationConfig {
        bool enableTransport;
        bool enableEconomy;
        bool enableConstruction;
        bool enableWorkers;

        uint32_t initialTick;
        uint32_t maxTicks;

        SimulationConfig()
            : enableTransport(true)
            , enableEconomy(false)
            , enableConstruction(false)
            , enableWorkers(false)
            , initialTick(0)
            , maxTicks(0)
        {
        }
    };

} // namespace World
