#pragma once
#include "../ConstructionManager.h"
#include "../../Core/EventBus.h"

namespace Logic { class EconomyManager; }

namespace World {

struct BuildCommand {
    BuildingType type;
    int tileX, tileY;
    Flag* entranceFlag;       // NULL = create new flag at calculated entrance position
    bool autoConnectRoad;     // whether to connect flag to road network and sync carriers

    BuildCommand() : type(Building_None), tileX(0), tileY(0), entranceFlag(NULL), autoConnectRoad(true) {}
};

class ConstructionSystem : public Core::EventListener {
public:
    ConstructionSystem();
    ~ConstructionSystem();

    void Initialize(FlagManager* flagManager, RoadManager* roadManager,
                    DemandManager* demandManager, CargoManager* cargoManager,
                    Flag* warehouseFlag, Core::EventBus* eventBus);

    void Enqueue(const BuildCommand& cmd);

    void Update(float dt);

    // PostUpdate checks for completed sites and broadcasts
    // Event_ConstructionComplete before they are removed.
    void PostUpdate();

    void GenerateRequests(Logic::EconomyManager* economy);

    void OnRoadRemoved(Road* road);

    ConstructionSite* GetSiteAt(int x, int y) const;
    ConstructionSite* GetSiteForFlag(const Flag* flag) const;
    Flag* FindConstructionDemand(Flag* fromFlag, ResourceType type) const;

    int GetActiveCount() const { return m_manager.GetCount(); }
    ConstructionSite* GetSite(int index) const { return m_manager.GetSite(index); }
    const std::vector<ConstructionSite*>& GetAllSites() const { return m_manager.GetAllSites(); }

    ConstructionManager* GetManager() { return &m_manager; }

    virtual void OnEvent(Core::EventType type, void* data);

private:
    ConstructionManager m_manager;
    FlagManager* m_flagManager;
    RoadManager* m_roadManager;
    DemandManager* m_demandManager;
    CargoManager* m_cargoManager;
    Flag* m_warehouseFlag;
    Core::EventBus* m_eventBus;
    bool m_initialized;

    std::vector<ConstructionSite*> m_completed; // sites already reported as complete (prevents double-fire)
};

} // namespace World
