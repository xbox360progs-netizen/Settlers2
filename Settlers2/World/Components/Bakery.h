#ifndef WORLD_COMPONENTS_BAKERY_H
#define WORLD_COMPONENTS_BAKERY_H

#include "ProductionBuilding.h"

namespace World {

class Bakery : public ProductionBuilding {
public:
    Bakery(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::Bakery, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_Flour);
        m_rules[0].AddInput(ResourceType_Water);
        m_rules[0].AddOutput(ResourceType_Bread);
    }
};

} // namespace World

#endif
