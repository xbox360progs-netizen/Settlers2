#pragma once
#include "../Interfaces/IRoadGraph.h"
#include "TransportRoute.h"

namespace World {

    static const int kRoadGraphMaxFlags = 512;

    class RoadGraph : public IRoadGraph {
    public:
        RoadGraph();

        virtual bool FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute);

        void AddEdge(FlagId a, FlagId b);
        void RemoveEdge(FlagId a, FlagId b);
        bool HasEdge(FlagId a, FlagId b) const;

        static int GetMaxFlags() { return kRoadGraphMaxFlags; }

    private:
        bool m_adj[kRoadGraphMaxFlags][kRoadGraphMaxFlags];
    };

} // namespace World
