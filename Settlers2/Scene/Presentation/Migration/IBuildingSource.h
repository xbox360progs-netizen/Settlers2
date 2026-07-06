#pragma once
#include <stdint.h>
#include "BuildingView.h"

// Temporary abstraction used only during World → SimulationCore migration.
// Remove after LegacyBuildingSource is deleted.

namespace Scene {

class IBuildingSource {
public:
    virtual ~IBuildingSource() {}

    virtual uint32_t GetBuildingCount() const = 0;
    virtual bool GetBuilding(uint32_t index, BuildingView& out) const = 0;
};

class IConstructionSiteSource {
public:
    virtual ~IConstructionSiteSource() {}

    virtual uint32_t GetConstructionSiteCount() const = 0;
    virtual bool GetConstructionSite(uint32_t index, BuildingView& out) const = 0;
};

} // namespace Scene
