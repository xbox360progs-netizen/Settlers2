#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"
#include "../Transport/TransportTypes.h"

namespace World {

    class DemandManager;
    struct WorldModel;

    class RenewableResourceSystem;

    class ProductionSystem : public ISimulationSystem {
    public:
        ProductionSystem();
        ~ProductionSystem();

        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }
        void SetRenewableSystem(RenewableResourceSystem* rs) { m_renewableSystem = rs; }
        void SetConsumptionEnabled(bool e) { m_consumptionEnabled = e; }
        void Tick(WorldModel& world);

    private:
        void ProcessProduction(WorldModel& world);
        void HandleDeliveryEvents(WorldModel& world);

        DemandManager* m_demandManager;
        RenewableResourceSystem* m_renewableSystem;
        bool m_consumptionEnabled;
        uint32_t m_tickCount;
        static const FlagId kProductionFlagBase;
    };

} // namespace World