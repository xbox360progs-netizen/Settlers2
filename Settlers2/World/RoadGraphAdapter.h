#pragma once
#include "../SimulationCore/Interfaces/IRoadGraph.h"

namespace World {
    class RoadManager;
    class FlagManager;

    class RoadGraphAdapter : public IRoadGraph {
    public:
        RoadGraphAdapter(RoadManager& roads, FlagManager& flags);
        bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute) override;
    private:
        RoadManager& m_roads;
        FlagManager& m_flags;
    };
}
