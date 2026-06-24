#ifndef WORLD_COMPONENTS_BRONZESMELTER_H
#define WORLD_COMPONENTS_BRONZESMELTER_H

#include "ProductionBuilding.h"

namespace World {

class BronzeSmelter : public ProductionBuilding {
public:
    BronzeSmelter(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::BronzeSmelter, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_BronzeOre);
        m_rules[0].AddInput(ResourceType_Coal);
        m_rules[0].AddOutput(ResourceType_BronzeBar);
        SyncIOFromRules();
    }
};

} // namespace World

#endif
