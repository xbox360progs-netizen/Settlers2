#pragma once
#include "IBuildingSource.h"

// Temporary — reads SimulationCore WorldModel for migrated building data.
// Remove after WorldModel is the sole source and LegacyBuildingSource is deleted.

namespace World {
    struct WorldModel;
}

namespace Scene {

class SimulationCoreBuildingSource : public IBuildingSource, public IConstructionSiteSource {
public:
    void SetWorldModel(const World::WorldModel* world);

    // IBuildingSource
    virtual uint32_t GetBuildingCount() const;
    virtual bool GetBuilding(uint32_t index, BuildingView& out) const;

    // IConstructionSiteSource
    virtual uint32_t GetConstructionSiteCount() const;
    virtual bool GetConstructionSite(uint32_t index, BuildingView& out) const;

private:
    // Maps SimulationCore BuildingType → legacy BuildingType enum value for renderer.
    static uint8_t MapBuildingType(uint8_t simType);

    const World::WorldModel* m_world;
};

} // namespace Scene
