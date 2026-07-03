#pragma once
#include "../Core/CommandBus.h"
#include "../Core/EventBus.h"
#include <vector>

namespace Logic {
    class EconomyManager;
}

namespace World {
    class Carrier;
    class CarrierManager;
    class CargoManager;
    class ConstructionManager;
    class Flag;
    class FlagManager;
    class Map;
    struct Road;
    class RoadManager;
    class ObjectLifecycleManager : public Core::CommandListener {
    public:
        ObjectLifecycleManager(FlagManager* fm, RoadManager* rm, CarrierManager* cm,
                               CargoManager* cargoMgr,
                               ConstructionManager* con, Logic::EconomyManager* em,
                               Map* map);

        void SetEventBus(Core::EventBus* bus) { m_eventBus = bus; }

        // Safe delete — returns false if the object cannot be safely destroyed
        virtual void OnCommand(Core::CommandType type, void* data);

        bool SafeDeleteFlag(Flag* flag);
        bool SafeDeleteRoad(Road* road);
        bool SafeDeleteCarrier(Carrier* carrier);
        bool SafeDeleteBuilding(class Building* building);

        // Force delete — cancels/interrupts dependencies, then destroys
        void ForceDeleteFlag(Flag* flag);
        void ForceDeleteRoad(Road* road);
        void ForceDeleteCarrier(Carrier* carrier);
        void ForceDeleteBuilding(class Building* building, class Map* map = NULL);

        // Deferred deletion: mark as pending, then flush at end of frame
        void FlushDeletions();

    private:
        FlagManager* m_flagManager;
        RoadManager* m_roadManager;
        CarrierManager* m_carrierManager;
        CargoManager* m_cargoManager;
        ConstructionManager* m_constructionManager;
        Logic::EconomyManager* m_economyManager;
        Map* m_map;
        Core::EventBus* m_eventBus;

        std::vector<Flag*> m_pendingFlags;
    };
}
