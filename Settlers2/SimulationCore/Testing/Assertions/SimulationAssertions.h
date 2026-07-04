#pragma once
#include "TransportAssertions.h"
#include "ConstructionAssertions.h"
#include "WorldAssertions.h"

// AllInvariants — facade that runs every category and accumulates failures.
// ConstructionAssertions::NoSiteStuckForever needs currentTick; overloads provided.

namespace World { namespace Assert {

    inline bool AllInvariants(const WorldModel& world)
    {
        bool ok = true;
        ok = NoOverDeliveredResources(world) && ok;
        ok = NoExcessTransportRequests(world) && ok;
        ok = NoOrphanedTransportRequests(world) && ok;
        ok = NoInvalidHandles(world) && ok;
        return ok;
    }

    inline bool AllInvariants(const WorldModel& world, uint32_t currentTick)
    {
        bool ok = AllInvariants(world);
        ok = NoSiteStuckForever(world, currentTick) && ok;
        return ok;
    }

}} // namespace World::Assert
