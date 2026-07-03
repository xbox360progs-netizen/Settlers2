#pragma once
#include <stdio.h>
#include "../Settlers2/SimulationCore/Simulation/SimulationState.h"

inline void PrintTelemetry(const World::SimulationState& state)
{
    printf("[SIM] tick=%u transport_a=%d transport_b=%d"
        " econ_pend=%u econ_ful=%u"
        " constr_pend=%u constr_active=%u constr_done=%u"
        " world=%s\n",
        state.tickCount,
        state.activeTransportTasks,
        state.blockedTransportTasks,
        state.economyPendingRequests,
        state.economyFulfilledRequests,
        state.constructionPendingRequests,
        state.constructionActiveSites,
        state.constructionCompletedSites,
        state.worldLoaded ? "loaded" : "empty");
}

inline void PrintHeader()
{
    printf("=== SimulationRunner ===\n");
    printf("No graphics, no input, no audio — headless simulation only.\n\n");
}
