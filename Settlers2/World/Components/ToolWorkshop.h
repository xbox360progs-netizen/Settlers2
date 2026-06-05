#ifndef WORLD_COMPONENTS_TOOLWORKSHOP_H
#define WORLD_COMPONENTS_TOOLWORKSHOP_H

#include "ProductionBuilding.h"

namespace World {

class ToolWorkshop : public ProductionBuilding {
public:
    ToolWorkshop(int x, int y, uint8_t o, Map* m)
        : ProductionBuilding(BuildingType::ToolWorkshop, x, y, o, m)
    {
        m_numRules = 1;
        m_rules[0].AddInput(ResourceType_Wood);
        m_rules[0].AddInput(ResourceType_IronBar);
        m_rules[0].AddOutput(ResourceType_Trap);
    }
};

} // namespace World

#endif
