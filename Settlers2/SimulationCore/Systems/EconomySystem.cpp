#include "EconomySystem.h"
#include "../World/WorldModel.h"

namespace World {

    EconomySystem::EconomySystem()
        : m_tickCount(0)
    {
    }

    EconomySystem::~EconomySystem()
    {
    }

    void EconomySystem::Tick(WorldModel& world)
    {
        ++m_tickCount;

        GenerateDemands(world);

        // Update state snapshot
        uint32_t pending = 0;
        uint32_t fulfilled = 0;
        for (int i = 0; i < world.pendingRequestCount; ++i) {
            if (world.pendingRequests[i].fulfilled)
                ++fulfilled;
            else
                ++pending;
        }
        m_state.pendingRequests = pending;
        m_state.fulfilledRequests = fulfilled;
    }

    void EconomySystem::GenerateDemands(WorldModel& world)
    {
        // Every 50 ticks, generate a sample wood transport demand.
        // Placeholder — real demand logic comes later.
        if ((m_tickCount % 50) != 0) return;
        if (world.pendingRequestCount >= kMaxPendingRequests) return;

        TransportRequest& req = world.pendingRequests[world.pendingRequestCount++];
        req.resource = ResourceType_Wood;
        req.origin = 1;
        req.destination = 5;
        req.reason = TTR_Production;
        req.fulfilled = false;

        ++m_state.totalDemandsGenerated;
    }

} // namespace World
