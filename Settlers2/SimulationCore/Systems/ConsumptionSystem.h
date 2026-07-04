#pragma once
#include <stdint.h>
#include "ISimulationSystem.h"
#include "../Transport/TransportTypes.h"

namespace World {

    class DemandManager;
    struct WorldModel;

    class ConsumptionSystem : public ISimulationSystem {
    public:
        ConsumptionSystem();
        ~ConsumptionSystem();

        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }
        void Tick(WorldModel& world);

        bool IsMineFed(int buildingIndex, const WorldModel& world) const;

    private:
        void ProcessMineFood(WorldModel& world);
        int GetFoodCycleTimer(int buildingIndex) const;

        DemandManager* m_demandManager;
        uint32_t m_tickCount;
        static const int kMaxMines;
        int m_foodCycleTimers[32];
        static const FlagId kConsumptionFlagBase;
    };

}
