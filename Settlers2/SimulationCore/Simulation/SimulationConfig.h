#pragma once
#include <stdint.h>

namespace World {

    struct SimulationConfig {
        bool enableTransport;
        bool enableEconomy;
        bool enableConstruction;
        bool enableWorkers;
        bool enableProduction;
        bool enableWarehouse;
        bool enableSettlement;
        bool enableTreeDepletion;
        bool enableConsumption;

        uint32_t initialTick;
        uint32_t maxTicks;

        SimulationConfig()
            : enableTransport(true)
            , enableEconomy(false)
            , enableConstruction(false)
            , enableWorkers(false)
            , enableProduction(false)
            , enableWarehouse(false)
            , enableSettlement(false)
            , enableTreeDepletion(false)
            , enableConsumption(false)
            , initialTick(0)
            , maxTicks(0)
        {
        }
    };

} // namespace World
