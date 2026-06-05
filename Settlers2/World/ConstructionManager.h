#pragma once
#include <vector>
#include "ConstructionSite.h"

namespace Logic { class EconomyManager; }

namespace World {

    class ConstructionManager {
    public:
        ConstructionManager();
        ~ConstructionManager();

        void AddSite(ConstructionSite* site);
        void RemoveSite(ConstructionSite* site);
        void RemoveSiteAt(int x, int y);
        void Update(float dt);

        void GenerateRequests(Logic::EconomyManager* economy);

        ConstructionSite* GetSiteAt(int x, int y) const;
        ConstructionSite* GetSiteForFlag(const Flag* flag) const;

        size_t GetCount() const { return m_sites.size(); }
        ConstructionSite* GetSite(size_t index) const { return (index < m_sites.size()) ? m_sites[index] : NULL; }
        const std::vector<ConstructionSite*>& GetAllSites() const { return m_sites; }

        void Clear();

    private:
        std::vector<ConstructionSite*> m_sites;
    };

}
