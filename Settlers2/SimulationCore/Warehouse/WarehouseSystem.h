#pragma once
#include <stdint.h>
#include "../Systems/ISimulationSystem.h"
#include "../Core/ResourceTypes.h"
#include "../Transport/TransportTypes.h"

namespace World {

    class DemandManager;
    struct WorldModel;

    struct StockpileEntry {
        ResourceType type;
        int amount;

        StockpileEntry() : type(ResourceType_None), amount(0) {}
    };

    class WarehouseSystem : public ISimulationSystem {
    public:
        WarehouseSystem();
        ~WarehouseSystem();

        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }
        virtual void Tick(WorldModel& world);

        // Testability
        int GetStockpileCount() const { return m_stockpileCount; }
        int GetStockpileAmount(ResourceType type) const;
        static const FlagId kWarehouseFlag;

    private:
        void HandleDeliveryEvents(WorldModel& world);
        void ScanProductionBuffers(WorldModel& world);

        DemandManager* m_demandManager;
        uint32_t m_tickCount;

        static const int kMaxStockpile = 32;
        StockpileEntry m_stockpile[kMaxStockpile];
        int m_stockpileCount;
    };

} // namespace World
