#pragma once
#include <stdio.h>
#include "../World/WorldModel.h"
#include "../Simulation/SimulationState.h"
#include "../Transport/TransportTypes.h"

namespace World {

    struct Simulation;

    namespace Assert {

        inline bool NoOverDeliveredResources(const WorldModel& world)
        {
            for (int i = 0; i < world.activeSiteCount; ++i) {
                const ConstructionSite& site = world.activeSites[i];
                for (int r = 0; r < site.resourceCount; ++r) {
                    if (site.resources[r].delivered > site.resources[r].required) {
                        printf("[FAIL] Site %d resource %d: delivered %d > required %d\n",
                            i, r, site.resources[r].delivered, site.resources[r].required);
                        return false;
                    }
                }
            }
            return true;
        }

        // Resource-level invariant: pending requests for a resource must not exceed
        // total undelivered demand for that resource across all construction sites.
        inline bool NoExcessTransportRequests(const WorldModel& world)
        {
            for (int res = 1; res < ResourceType_Count; ++res) {
                int needed = 0;
                int delivered = 0;
                int pending = 0;

                for (int s = 0; s < world.activeSiteCount; ++s) {
                    const ConstructionSite& site = world.activeSites[s];
                    for (int r = 0; r < site.resourceCount; ++r) {
                        if (site.resources[r].resource == static_cast<ResourceType>(res)) {
                            needed += site.resources[r].required;
                            delivered += site.resources[r].delivered;
                        }
                    }
                }

                for (int p = 0; p < world.pendingRequestCount; ++p) {
                    const TransportRequest& req = world.pendingRequests[p];
                    if (!req.fulfilled && req.resource == static_cast<ResourceType>(res)
                        && req.reason == TTR_Construction) {
                        pending++;
                    }
                }

                if (pending > needed - delivered) {
                    printf("[FAIL] Resource %d: %d pending, %d needed, %d delivered\n",
                        res, pending, needed, delivered);
                    return false;
                }
            }
            return true;
        }

        // Slot-level invariant: no slot reports requested=true when already fully delivered.
        inline bool NoOrphanedTransportRequests(const WorldModel& world)
        {
            for (int s = 0; s < world.activeSiteCount; ++s) {
                const ConstructionSite& site = world.activeSites[s];
                for (int r = 0; r < site.resourceCount; ++r) {
                    if (site.resources[r].requested && site.resources[r].delivered >= site.resources[r].required) {
                        printf("[FAIL] Site %d resource %d: requested but already fully delivered\n", s, r);
                        return false;
                    }
                }
            }
            return true;
        }

        inline bool ConstructionCountsMatch(const WorldModel& world,
            int expectedPending, int expectedActive, int expectedCompleted)
        {
            int pending = 0;
            int active = 0;
            int completed = 0;
            for (int i = 0; i < world.pendingConstructionCount; ++i) {
                if (!world.pendingConstructionRequests[i].fulfilled) pending++;
            }
            for (int i = 0; i < world.activeSiteCount; ++i) {
                if (world.activeSites[i].state == CS_Completed) completed++;
                else active++;
            }
            if (pending != expectedPending || active != expectedActive
                || completed != expectedCompleted) {
                printf("[FAIL] Construction counts: pending=%d (exp %d) active=%d (exp %d) completed=%d (exp %d)\n",
                    pending, expectedPending, active, expectedActive,
                    completed, expectedCompleted);
                return false;
            }
            return true;
        }

        inline bool AllInvariants(const WorldModel& world)
        {
            bool ok = true;
            ok = NoOverDeliveredResources(world) && ok;
            ok = NoExcessTransportRequests(world) && ok;
            ok = NoOrphanedTransportRequests(world) && ok;
            return ok;
        }

    } // namespace Assert

} // namespace World
