#pragma once
#include <stdint.h>

namespace World {

    struct WorldModel;

    class EconomySystem {
    public:
        EconomySystem();
        ~EconomySystem();

        void Tick(WorldModel& world);

        struct State {
            uint32_t pendingRequests;
            uint32_t fulfilledRequests;
            uint32_t totalDemandsGenerated;

            State()
                : pendingRequests(0)
                , fulfilledRequests(0)
                , totalDemandsGenerated(0)
            {
            }
        };

        const State& GetState() const { return m_state; }

    private:
        void GenerateDemands(WorldModel& world);

        State m_state;
        uint32_t m_tickCount;
    };

} // namespace World
