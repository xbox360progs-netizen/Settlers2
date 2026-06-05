#ifndef WORLD_COMPONENTS_SAWMILL_H
#define WORLD_COMPONENTS_SAWMILL_H

#include "ProductionBuilding.h"
#include "../Map.h"

namespace World {

class Sawmill : public ProductionBuilding {
public:
    Sawmill(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::Sawmill, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_Wood);
        m_rules[0].AddOutput(ResourceType_Planks);
    }
};

} // namespace World

#endif
