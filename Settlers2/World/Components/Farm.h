#ifndef WORLD_COMPONENTS_FARM_H
#define WORLD_COMPONENTS_FARM_H

#include "Building.h"
#include "../Map.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Farm : public Building {
public:
    Farm(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Farm, x, y, o, m) {
        m_productionInterval = 4.0f;
        outputResources.push_back(ResourceType_Wheat);
    }

    bool CanProduce() override {
        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry)
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Field, pos, m_target);
        }
        return m_hasTarget && !IsOutputFull();
    }

    bool ProduceOne() override {
        return AddOutput(ResourceType_Wheat, 1);
    }
};

} // namespace World

#endif
