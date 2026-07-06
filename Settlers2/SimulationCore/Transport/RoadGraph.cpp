#include <cstring>
#include <cassert>
#include "RoadGraph.h"

namespace World {

    RoadGraph::RoadGraph()
    {
        std::memset(m_adj, 0, sizeof(m_adj));
    }

    void RoadGraph::AddEdge(FlagId a, FlagId b)
    {
        assert(a < kRoadGraphMaxFlags);
        assert(b < kRoadGraphMaxFlags);
        m_adj[a][b] = true;
        m_adj[b][a] = true;
    }

    void RoadGraph::RemoveEdge(FlagId a, FlagId b)
    {
        assert(a < kRoadGraphMaxFlags);
        assert(b < kRoadGraphMaxFlags);
        m_adj[a][b] = false;
        m_adj[b][a] = false;
    }

    bool RoadGraph::HasEdge(FlagId a, FlagId b) const
    {
        if (a >= kRoadGraphMaxFlags) return false;
        if (b >= kRoadGraphMaxFlags) return false;
        return m_adj[a][b];
    }

    bool RoadGraph::FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute)
    {
        if (source >= kRoadGraphMaxFlags) return false;
        if (destination >= kRoadGraphMaxFlags) return false;
        if (source == destination) return false;

        bool visited[kRoadGraphMaxFlags];
        FlagId prev[kRoadGraphMaxFlags];
        FlagId queue[kRoadGraphMaxFlags];

        std::memset(visited, 0, sizeof(visited));
        for (int i = 0; i < kRoadGraphMaxFlags; ++i) {
            prev[i] = kRoadGraphMaxFlags;
        }

        int qHead = 0;
        int qTail = 0;

        queue[qTail++] = source;
        visited[source] = true;

        while (qHead < qTail) {
            FlagId cur = queue[qHead++];
            if (cur == destination) break;

            for (FlagId next = 0; next < kRoadGraphMaxFlags; ++next) {
                if (m_adj[cur][next] && !visited[next]) {
                    visited[next] = true;
                    prev[next] = cur;
                    queue[qTail++] = next;
                }
            }
        }

        if (!visited[destination]) return false;

        // Reconstruct path from destination back to source
        FlagId path[kMaxRouteLength];
        int pathLen = 0;
        FlagId cur = destination;
        while (cur != source) {
            path[pathLen++] = cur;
            cur = prev[cur];
        }
        path[pathLen++] = source;

        // Reverse so route is source → ... → destination
        outRoute.count = static_cast<uint8_t>(pathLen);
        for (int i = 0; i < pathLen; ++i) {
            outRoute.flags[i] = path[pathLen - 1 - i];
        }

        return true;
    }

} // namespace World
