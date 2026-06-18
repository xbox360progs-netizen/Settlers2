#pragma once
#include <stdint.h>

namespace World {
    class Flag;
    class FlagManager;
    class RoadManager;
    class CarrierManager;
    struct Road;
    class Warehouse;

    class TransportJobManager {
    public:
        TransportJobManager();
        ~TransportJobManager();

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetCarrierManager(CarrierManager* cm) { m_carrierManager = cm; }
        void SetWarehouse(Warehouse* wh) { m_warehouse = wh; }

        // All stubs: TransportJob system replaced by Cargo/Demand.
        bool IsRoadInUse(Road* road) const { return false; }
        bool IsFlagInUse(Flag* flag) const { return false; }

    private:
        FlagManager* m_flagManager;
        RoadManager* m_roadManager;
        CarrierManager* m_carrierManager;
        Warehouse* m_warehouse;
    };
}
