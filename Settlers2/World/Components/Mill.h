#ifndef WORLD_COMPONENTS_MILL_H
#define WORLD_COMPONENTS_MILL_H

#include "ProductionBuilding.h"

namespace World {

class Mill : public ProductionBuilding {
public:
    Mill(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::Mill, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_Wheat);
        m_rules[0].AddOutput(ResourceType_Flour);
        SyncIOFromRules();
    }
};

} // namespace World

#endif
