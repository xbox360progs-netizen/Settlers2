#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "../Settlers2/SimulationCore/Simulation/Simulation.h"
#include "../Settlers2/SimulationCore/Simulation/SimulationConfig.h"
#include "../Settlers2/SimulationCore/Simulation/SimulationState.h"
#include "../Settlers2/SimulationCore/Construction/ConstructionSystem.h"
#include "../Settlers2/SimulationCore/Worker/WorkerSystem.h"
#include "../Settlers2/SimulationCore/Testing/TestScenario.h"
#include "RunnerConfig.h"
#include "ConsoleTelemetry.h"
#include "ScenarioLoader.h"

static int RunDefault()
{
    PrintHeader();

    RunnerConfig runCfg;
    World::SimulationConfig simCfg;
    simCfg.enableEconomy = true;
    World::Simulation simulation(simCfg);

    simulation.AddSystem(new World::ConstructionSystem());
    simulation.AddSystem(new World::WorkerSystem());

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

static int RunScenario(const char* name)
{
    printf("=== SimulationRunner — Scenario Mode ===\n\n");

    World::SimulationConfig simCfg;
    simCfg.enableEconomy = true;
    World::Simulation simulation(simCfg);

    simulation.AddSystem(new World::ConstructionSystem());
    simulation.AddSystem(new World::WorkerSystem());

    World::WorldModel world = Scenarios::CreateEmptyWorld();
    simulation.LoadWorld(world);

    bool passed = World::RunScenario(name, simulation, world);

    printf("\nFinal state:\n");
    PrintTelemetry(simulation.GetState());

    return passed ? 0 : 1;
}

int main(int argc, char* argv[])
{
    if (argc > 1 && strcmp(argv[1], "--scenario") == 0) {
        if (argc > 2) {
            return RunScenario(argv[2]);
        }
        printf("Usage: SimulationRunner --scenario <name>\n");
        World::ListScenarios();
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("Usage:\n");
        printf("  SimulationRunner              — run default tick loop\n");
        printf("  SimulationRunner --scenario T1 — run scenario\n");
        World::ListScenarios();
        return 0;
    }

    return RunDefault();
}
