#pragma once
#include "../../Core/EventBus.h"

namespace Logic { class EconomyManager; }

namespace World {
    class FlagManager;
    class RoadManager;
    class CargoManager;
    class StorehouseManager;
    class DemandManager;
    class Flag;
    class Warehouse;
    class Building;
}

namespace World {

class EconomySystem : public Core::EventListener {
public:
    EconomySystem();
    ~EconomySystem();

    void Initialize(Core::EventBus* eventBus);

    // External manager mode: accept an already-created EconomyManager
    void SetManager(Logic::EconomyManager* mgr);

    Logic::EconomyManager* GetManager() { return m_manager; }
    const Logic::EconomyManager* GetManager() const { return m_manager; }

    void Update(float dt);
    void CollectWarehouse();

    void SetFlagManager(FlagManager* fm);
    void SetRoadManager(RoadManager* rm);
    void SetCargoManager(CargoManager* cm);
    void SetStorehouseManager(StorehouseManager* sm);
    void SetDemandManager(DemandManager* dm);

    void AddBuilding(Building* building);
    void RemoveBuilding(Building* building);
    void SetWarehouse(Warehouse* warehouse);

    Warehouse* GetWarehouse() const;
    StorehouseManager* GetStorehouseManager() const;

    void RequestResource(Building* requester, int type, int amount, int priority);
    void RequestConstructionResource(Flag* destFlag, int type, int amount, int priority);

    int GetTotalStock(int type) const;
    int GetCargoInTransit(int type) const;
    int GetCargoOnFlags(int type) const;

    virtual void OnEvent(Core::EventType type, void* data);

private:
    Logic::EconomyManager* m_manager;
    bool m_ownsManager;
    Core::EventBus* m_eventBus;
    DemandManager* m_demandManager;
};

} // namespace World
