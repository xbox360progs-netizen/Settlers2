#pragma once
#include "../../SimulationCore/Interfaces/IRoadGraph.h"
#include "../../SimulationCore/Transport/TransportRoute.h"

namespace World {

    class FlagManager;
    class RoadManager;

    class RoadGraphAdapter : public IRoadGraph {
    public:
        RoadGraphAdapter(RoadManager& roads, FlagManager& flags);
        virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute);

    private:
        RoadManager& m_roads;
        FlagManager& m_flags;
    };

} // namespace World
