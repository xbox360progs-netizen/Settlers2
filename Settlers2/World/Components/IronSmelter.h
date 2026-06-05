#ifndef WORLD_COMPONENTS_IRONSMELTER_H
#define WORLD_COMPONENTS_IRONSMELTER_H

#include "ProductionBuilding.h"

namespace World {

class IronSmelter : public ProductionBuilding {
public:
    IronSmelter(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::IronSmelter, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_IronOre);
        m_rules[0].AddInput(ResourceType_Coal);
        m_rules[0].AddOutput(ResourceType_IronBar);
        SyncIOFromRules();
    }
};

} // namespace World

#endif
