#pragma once
#include <stdio.h>
#include "../../World/WorldModel.h"

namespace World { namespace Assert {

    inline bool NoOverDeliveredResources(const WorldModel& world)
    {
        for (int i = 0; i < world.activeSiteCount; ++i) {
            const ConstructionSite& site = world.activeSites[i];
            for (int r = 0; r < site.resourceCount; ++r) {
                if (site.resources[r].delivered > site.resources[r].required) {
                    printf("[FAIL][Construction] Site %d resource %d: delivered %d > required %d\n",
                        i, r, site.resources[r].delivered, site.resources[r].required);
                    return false;
                }
            }
        }
        return true;
    }

    // A site must not stay in the same non-terminal, non-blocked state indefinitely.
    // CS_WaitingForResources is excluded — it legitimately waits on the road graph.
    inline bool NoSiteStuckForever(const WorldModel& world, uint32_t currentTick)
    {
        static const uint32_t kStuckThreshold = 50000;

        for (int i = 0; i < world.activeSiteCount; ++i) {
            const ConstructionSite& site = world.activeSites[i];
            if (site.state == CS_Completed) continue;
            if (site.state == CS_WaitingForResources) continue;

            uint32_t elapsed = currentTick - site.lastStateChangeTick;
            if (elapsed > kStuckThreshold) {
                printf("[FAIL][Construction] Site %d stuck in state %d for %u ticks\n",
                    i, static_cast<int>(site.state), elapsed);
                return false;
            }
        }
        return true;
    }

}} // namespace World::Assert
