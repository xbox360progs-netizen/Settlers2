#ifndef WORLD_COMPONENTS_GOLDSMELTER_H
#define WORLD_COMPONENTS_GOLDSMELTER_H

#include "ProductionBuilding.h"

namespace World {

class GoldSmelter : public ProductionBuilding {
public:
    GoldSmelter(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::GoldSmelter, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_GoldOre);
        m_rules[0].AddInput(ResourceType_Coal);
        m_rules[0].AddOutput(ResourceType_GoldBar);
        SyncIOFromRules();
    }
};

} // namespace World

#endif
