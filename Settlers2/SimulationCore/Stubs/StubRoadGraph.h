#pragma once
#include "../Interfaces/IRoadGraph.h"

namespace World {

    class StubRoadGraph : public IRoadGraph {
    public:
        StubRoadGraph() {}
        virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute)
        {
            (void)source;
            (void)destination;
            (void)outRoute;
            return false; // No roads in headless mode
        }
    };

} // namespace World
