#include <stdio.h>
#include <stdint.h>
#include "../Settlers2/SimulationCore/Simulation/Simulation.h"
#include "../Settlers2/SimulationCore/Simulation/SimulationConfig.h"
#include "../Settlers2/SimulationCore/Simulation/SimulationState.h"
#include "RunnerConfig.h"
#include "ConsoleTelemetry.h"
#include "ScenarioLoader.h"

int main()
{
    PrintHeader();

    RunnerConfig runCfg;
    World::SimulationConfig simCfg;
    simCfg.enableEconomy = true;
    World::Simulation simulation(simCfg);

    World::WorldModel world = Scenarios::CreateEmptyWorld();
    simulation.LoadWorld(world);

    printf("Running %u ticks, telemetry every %u ticks...\n\n",
        runCfg.tickCount, runCfg.telemetryInterval);

    for (uint32_t tick = 0; tick < runCfg.tickCount; ++tick)
    {
        simulation.Tick();

        if ((tick % runCfg.telemetryInterval) == 0)
            PrintTelemetry(simulation.GetState());
    }

    printf("\nDone. Final state:\n");
    PrintTelemetry(simulation.GetState());

    return 0;
}
