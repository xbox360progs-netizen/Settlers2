#pragma once
#include <stdio.h>
#include "../Settlers2/SimulationCore/Simulation/SimulationState.h"

inline void PrintTelemetry(const World::SimulationState& state)
{
    printf("[SIM] tick=%u transport_active=%d transport_blocked=%d"
        " economy_pending=%u economy_fulfilled=%u world=%s\n",
        state.tickCount,
        state.activeTransportTasks,
        state.blockedTransportTasks,
        state.economyPendingRequests,
        state.economyFulfilledRequests,
        state.worldLoaded ? "loaded" : "empty");
}

inline void PrintHeader()
{
    printf("=== SimulationRunner ===\n");
    printf("No graphics, no input, no audio — headless simulation only.\n\n");
}
