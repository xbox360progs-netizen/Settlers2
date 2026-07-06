#pragma once
#include "IBuildingSource.h"

// Temporary — wraps legacy World managers for migration.
// Remove when all PresentationSystems read from SimulationCore.

namespace World {
    class FlagManager;
    class ConstructionManager;
}

namespace Scene {

class LegacyBuildingSource : public IBuildingSource, public IConstructionSiteSource {
public:
    void SetManagers(
        World::FlagManager* flagManager,
        World::ConstructionManager* constructionManager
    );

    // IBuildingSource
    virtual uint32_t GetBuildingCount() const;
    virtual bool GetBuilding(uint32_t index, BuildingView& out) const;

    // IConstructionSiteSource
    virtual uint32_t GetConstructionSiteCount() const;
    virtual bool GetConstructionSite(uint32_t index, BuildingView& out) const;

private:
    World::FlagManager*         m_flagManager;
    World::ConstructionManager* m_constructionManager;
};

} // namespace Scene
