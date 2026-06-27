#pragma once
#include "ConstructionFactory.h"
#include "RoadNetworkRelinker.h"
#include "BuildContext.h"
#include "../ConstructionManager.h"
#include "../../Core/EventBus.h"
#include "../../Core/CommandBus.h"

namespace Logic { class EconomyManager; }
class CarrierManager;
class Map;

namespace World {

class ConstructionSystem : public Core::EventListener, public Core::CommandListener {
public:
    ConstructionSystem();
    ~ConstructionSystem();

    void Initialize(const BuildContext& ctx, Core::EventBus* eventBus, Core::CommandBus* commandBus);

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

    RoadNetworkRelinker* GetRelinker() { return &m_relinker; }

    virtual void OnEvent(Core::EventType type, void* data);
    virtual void OnCommand(Core::CommandType type, void* data);

private:
    void HandlePlaceFlag(const Core::PlaceFlagCmd& cmd);
    void SplitRoadAtFlag(Flag* flag);

    ConstructionManager m_manager;
    ConstructionFactory m_factory;
    RoadNetworkRelinker m_relinker;
    Core::EventBus* m_eventBus;
    Core::CommandBus* m_commandBus;
    CarrierManager* m_carriers;
    Map* m_map;
    bool m_initialized;

    // Use stable site IDs for double-fire guard (safe across delete/reuse)
    std::vector<ConstructionSiteId> m_completedIds;
};

} // namespace World
