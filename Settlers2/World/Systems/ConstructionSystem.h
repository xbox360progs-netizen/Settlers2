#pragma once
#include "ConstructionFactory.h"
#include "BuildContext.h"
#include "../ConstructionManager.h"
#include "../../Core/EventBus.h"

namespace Logic { class EconomyManager; }
class CarrierManager;
class Map;

namespace World {

class ConstructionSystem : public Core::EventListener {
public:
    ConstructionSystem();
    ~ConstructionSystem();

    void Initialize(const BuildContext& ctx, Core::EventBus* eventBus);

    void Enqueue(const BuildCommand& cmd);

    void Update(float dt);

    // PostUpdate detects completed sites and broadcasts Event_ConstructionComplete.
    // Call between Economy and EventDispatch phases.
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
    void HandlePlaceFlag(const Core::PlaceFlagData& cmd);
    void SplitRoadAtFlag(Flag* flag);
    void LinkFlagToRoadNetwork(Flag* flag);
    void SyncCarriersForFlag(Flag* flag);

    ConstructionManager m_manager;
    ConstructionFactory m_factory;
    Core::EventBus* m_eventBus;
    CarrierManager* m_carriers;
    Map* m_map;
    bool m_initialized;

    // Use stable site IDs for double-fire guard (safe across delete/reuse)
    std::vector<ConstructionSiteId> m_completedIds;
};

} // namespace World
