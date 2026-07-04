#pragma once
#include "../Interfaces/IRoadGraph.h"
#include "../Transport/TransportRoute.h"

namespace World {

    class DirectRouteRoadGraph : public IRoadGraph {
    public:
        DirectRouteRoadGraph() {}
        virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute) {
            if (source == destination) return false;
            outRoute.count = 2;
            outRoute.flags[0] = source;
            outRoute.flags[1] = destination;
            return true;
        }
    };

} // namespace World
