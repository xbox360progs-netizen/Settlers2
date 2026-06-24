#pragma once
#include "../ConstructionManager.h"
#include "../../Core/EventBus.h"

namespace Logic { class EconomyManager; }

namespace World {

struct BuildCommand {
    BuildingType buildingType;
    int posX, posY;
    uint32_t flagId;
    bool valid;

    BuildCommand() : buildingType(Building_None), posX(0), posY(0), flagId(0), valid(false) {}
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
    Core::EventBus* m_eventBus;
    bool m_initialized;
};

} // namespace World
