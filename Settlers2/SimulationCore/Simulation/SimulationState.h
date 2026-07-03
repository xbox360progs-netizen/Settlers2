#pragma once
#include <stdint.h>

namespace World {

    struct SimulationState {
        uint32_t tickCount;
        int activeTransportTasks;
        int blockedTransportTasks;
        uint32_t economyPendingRequests;
        uint32_t economyFulfilledRequests;
        uint32_t constructionPendingRequests;
        uint32_t constructionActiveSites;
        uint32_t constructionCompletedSites;
        bool worldLoaded;

        SimulationState()
            : tickCount(0)
            , activeTransportTasks(0)
            , blockedTransportTasks(0)
            , economyPendingRequests(0)
            , economyFulfilledRequests(0)
            , constructionPendingRequests(0)
            , constructionActiveSites(0)
            , constructionCompletedSites(0)
            , worldLoaded(false)
        {
        }
    };

} // namespace World
