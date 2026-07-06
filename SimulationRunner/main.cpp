#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "../Settlers2/SimulationCore/Simulation/Simulation.h"
#include "../Settlers2/SimulationCore/Simulation/SimulationConfig.h"
#include "../Settlers2/SimulationCore/Simulation/SimulationState.h"
#include "../Settlers2/SimulationCore/Testing/ScenarioRegistry.h"
#include "../Settlers2/SimulationCore/Testing/Scenarios/RegisterAll.h"
#include "RunnerConfig.h"
#include "ConsoleTelemetry.h"
#include "ScenarioLoader.h"

static int RunDefault()
{
    PrintHeader();

    RunnerConfig runCfg;
    World::SimulationConfig simCfg;
    simCfg.enableEconomy = true;
    simCfg.enableConstruction = true;
    simCfg.enableProduction = true;
    simCfg.enableWarehouse = true;
    simCfg.enableWorkers = true;
    simCfg.enableSettlement = true;
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

static int RunScenario(const char* name)
{
    printf("=== SimulationRunner — Scenario Mode ===\n\n");

    World::ISimulationScenario* scenario = World::ScenarioRegistry::Find(name);
    if (!scenario) {
        printf("Unknown scenario: %s\n", name);
        World::ScenarioRegistry::ListAll();
        return 1;
    }

    World::SimulationConfig simCfg;
    scenario->Configure(simCfg);
    World::Simulation simulation(simCfg);

    World::WorldModel world = Scenarios::CreateEmptyWorld();
    simulation.LoadWorld(world);

    scenario->Initialize(simulation);

    while (scenario->Tick(simulation))
    {
        simulation.Tick();
    }

    printf("\nFinal state:\n");
    PrintTelemetry(simulation.GetState());

    return 0;
}

// Remove --log and its argument from argv. Returns the new argument count.
static int RemoveLogArg(int argc, char* argv[])
{
    int originalArgc = argc;
    for (int i = 1; i < originalArgc - 1; ++i) {
        if (strcmp(argv[i], "--log") == 0) {
            FILE* newStdout = freopen(argv[i + 1], "w", stdout);
            if (newStdout != NULL) {
                printf("[log] Writing output to %s\n", argv[i + 1]);
            }
            // Shift remaining args left by 2 to remove --log <path>
            int shift = 2;
            for (int j = i; j + shift < originalArgc; ++j) {
                argv[j] = argv[j + shift];
            }
            argc -= 2;
            break;
        }
    }
    return argc;
}

int main(int argc, char* argv[])
{
    World::RegisterAllScenarios();

    argc = RemoveLogArg(argc, argv);

    if (argc > 1 && strcmp(argv[1], "--scenario") == 0) {
        if (argc > 2) {
            return RunScenario(argv[2]);
        }
        printf("Usage: SimulationRunner --scenario <name>\n");
        World::ScenarioRegistry::ListAll();
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "--help") == 0) {
        printf("Usage:\n");
        printf("  SimulationRunner                        — run default tick loop\n");
        printf("  SimulationRunner --log <file>           — with log file\n");
        printf("  SimulationRunner --scenario <name>      — run scenario\n");
        printf("  SimulationRunner --log <file> --scenario <name>  — both\n");
        World::ScenarioRegistry::ListAll();
        return 0;
    }

    return RunDefault();
}
