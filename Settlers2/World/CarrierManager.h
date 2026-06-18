#pragma once
#include <vector>
#include "Carrier.h"
#include "Road.h"
#include "Handle.h"
#include "../Core/Vector2i.h"

namespace World {
    class Flag;
    class FlagManager;
    class TransportJobManager;
    class RoadManager;
    class CarrierSystem;
    class DemandManager;
    class CargoManager;

    class CarrierManager {
    public:
        CarrierManager();
        void Update(float deltaTime);

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetJobManager(TransportJobManager* jm) { m_jobManager = jm; }
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetWarehouseFlag(Flag* f) { m_warehouseFlag = f; }
        void SetCarrierSystem(CarrierSystem* cs) { m_carrierSystem = cs; }
        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }
        void SetCargoManager(CargoManager* cm) { m_cargoManager = cm; }

        int GetCarrierCount() const { return (int)m_carriers.size(); }
        Carrier* GetCarrier(int index) const { return (index >= 0 && index < (int)m_carriers.size()) ? m_carriers[index] : NULL; }

        // Handle API
        CarrierHandle RegisterCarrier(Carrier* c);
        Carrier* ResolveCarrier(CarrierHandle h) const;
        CarrierHandle GetCarrierHandle(Carrier* c) const;
        void UnregisterCarrier(CarrierHandle h);

        void CreateCarrier(Road* road);
        void RemoveCarrier(Flag* a, Flag* b);
        void RemoveCarriersForFlag(Flag* f);
        void RemoveCarriersForRoad(Road* road);
        void SyncCarriersForRoad(Road* road);
        Carrier* GetCarrierForRoad(Road* road) const;
        bool IsRoadInUse(Road* road) const;
        bool IsFlagInUse(Flag* flag) const;

    private:
        std::vector<Carrier*> m_carriers;
        HandleRegistry m_carrierRegistry;
        FlagManager* m_flagManager;
        TransportJobManager* m_jobManager;
        RoadManager* m_roadManager;
        Flag* m_warehouseFlag;
        CarrierSystem* m_carrierSystem;
        DemandManager* m_demandManager;
        CargoManager* m_cargoManager;

        std::vector<Vector2i> BuildTransitPath(Flag* fromFlag, Flag* toFlag);
    };
}
