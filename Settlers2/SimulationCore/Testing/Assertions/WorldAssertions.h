#pragma once
#include <stdio.h>
#include "../../World/WorldModel.h"

namespace World { namespace Assert {

    inline bool NoInvalidHandles(const WorldModel& world)
    {
        for (int i = 0; i < world.activeSiteCount; ++i) {
            const ConstructionSite& site = world.activeSites[i];
            if (site.type < BuildingType_None || site.type >= BuildingType_Count) {
                printf("[FAIL][World] Site %d has invalid BuildingType %d\n",
                    i, static_cast<int>(site.type));
                return false;
            }
            if (site.state < CS_Pending || site.state > CS_Completed) {
                printf("[FAIL][World] Site %d has invalid ConstructionState %d\n",
                    i, static_cast<int>(site.state));
                return false;
            }
            for (int r = 0; r < site.resourceCount; ++r) {
                if (site.resources[r].resource < ResourceType_None || site.resources[r].resource >= ResourceType_Count) {
                    printf("[FAIL][World] Site %d resource %d has invalid ResourceType\n", i, r);
                    return false;
                }
                if (site.resources[r].required < 0 || site.resources[r].delivered < 0) {
                    printf("[FAIL][World] Site %d resource %d has negative count\n", i, r);
                    return false;
                }
            }
        }

        for (int p = 0; p < world.pendingRequestCount; ++p) {
            const TransportRequest& req = world.pendingRequests[p];
            if (req.resource < ResourceType_None || req.resource >= ResourceType_Count) {
                printf("[FAIL][World] Request %d has invalid ResourceType\n", p);
                return false;
            }
        }

        return true;
    }

}} // namespace World::Assert
