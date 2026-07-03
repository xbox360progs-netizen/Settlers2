#include "stdafx.h"
#include "RoadGraphAdapter.h"
#include "../FlagManager.h"
#include "../RoadManager.h"
#include "../Flag.h"
#include <vector>

namespace World {

    RoadGraphAdapter::RoadGraphAdapter(RoadManager& roads, FlagManager& flags)
        : m_roads(roads)
        , m_flags(flags)
    {
    }

    bool RoadGraphAdapter::FindRoute(FlagId source, FlagId destination, TransportRoute& outRoute)
    {
        Flag* srcFlag = m_flags.GetFlagById(source);
        Flag* dstFlag = m_flags.GetFlagById(destination);
        if (!srcFlag || !dstFlag)
            return false;

        std::vector<Flag*> path = m_roads.FindFlagPath(srcFlag, dstFlag);
        if (path.size() < 2 || path.size() > kMaxRouteLength)
            return false;

        outRoute.count = static_cast<uint8_t>(path.size());
        for (uint8_t i = 0; i < outRoute.count; ++i)
            outRoute.flags[i] = path[i]->id;

        return true;
    }

} // namespace World
