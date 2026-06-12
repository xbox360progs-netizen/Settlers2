#pragma once

namespace Logic {
    class EconomyManager;
}

namespace World {
    class Carrier;
    class CarrierManager;
    class ConstructionManager;
    class Flag;
    class FlagManager;
    struct Road;
    class RoadManager;
    class TransportJobManager;

    class ObjectLifecycleManager {
    public:
        ObjectLifecycleManager(FlagManager* fm, RoadManager* rm, CarrierManager* cm,
                               TransportJobManager* jm, ConstructionManager* con,
                               Logic::EconomyManager* em);

        // Safe delete — returns false if the object cannot be safely destroyed
        bool SafeDeleteFlag(Flag* flag);
        bool SafeDeleteRoad(Road* road);
        bool SafeDeleteCarrier(Carrier* carrier);
        bool SafeDeleteBuilding(class Building* building);

        // Force delete — cancels/interrupts dependencies, then destroys
        void ForceDeleteFlag(Flag* flag);
        void ForceDeleteRoad(Road* road);
        void ForceDeleteCarrier(Carrier* carrier);
        void ForceDeleteBuilding(class Building* building);

    private:
        FlagManager* m_flagManager;
        RoadManager* m_roadManager;
        CarrierManager* m_carrierManager;
        TransportJobManager* m_jobManager;
        ConstructionManager* m_constructionManager;
        Logic::EconomyManager* m_economyManager;
    };
}
