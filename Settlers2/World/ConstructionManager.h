#pragma once
#include <vector>
#include "ConstructionSite.h"

namespace Logic { class EconomyManager; }

namespace World {
    class DemandManager;
    class FlagManager;
    class RoadManager;

    class ConstructionManager {
    public:
        ConstructionManager();
        ~ConstructionManager();

        void AddSite(ConstructionSite* site);
        void RemoveSite(ConstructionSite* site);
        void RemoveSiteAt(int x, int y);
        void Update(float dt);

        void GenerateRequests(Logic::EconomyManager* economy);

        // Notify that a road was removed — recalculate affected builder routes
        void OnRoadRemoved(Road* road);

        // Mark all builder routes as dirty (recalculated next Update)
        void MarkBuilderRoutesDirty() { m_builderRoutesDirty = true; }
        bool IsBuilderRoutesDirty() const { return m_builderRoutesDirty; }
        void RecalculateBuilderRoutes();

        void SetFlagManager(FlagManager* fm) { m_flagManager = fm; }
        void SetRoadManager(RoadManager* rm) { m_roadManager = rm; }
        void SetWarehouseFlag(Flag* f) { m_warehouseFlag = f; }
        void SetDemandManager(DemandManager* dm) { m_demandManager = dm; }
        Flag* GetWarehouseFlag() const { return m_warehouseFlag; }

        ConstructionSite* GetSiteAt(int x, int y) const;
        ConstructionSite* GetSiteForFlag(const Flag* flag) const;

        // Find a construction flag reachable from fromFlag that needs the given resource.
        // Returns NULL if no construction demand exists.
        Flag* FindConstructionDemand(Flag* fromFlag, ResourceType type) const;

        size_t GetCount() const { return m_sites.size(); }
        ConstructionSite* GetSite(size_t index) const { return (index < m_sites.size()) ? m_sites[index] : NULL; }
        const std::vector<ConstructionSite*>& GetAllSites() const { return m_sites; }

        void Clear();

    private:
        void InitBuilderFirstLeg(ConstructionSite* site);

        std::vector<ConstructionSite*> m_sites;
        FlagManager* m_flagManager;
        RoadManager* m_roadManager;
        Flag* m_warehouseFlag;
        DemandManager* m_demandManager;
        bool m_builderRoutesDirty;
    };

}
