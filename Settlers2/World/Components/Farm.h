#ifndef WORLD_COMPONENTS_FARM_H
#define WORLD_COMPONENTS_FARM_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Farm : public Building {
public:
    Farm(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Farm, x, y, o, m) {
        outputResources.push_back(ResourceType_Wheat);
    }

    void Update() override {
        if (m_storage[ResourceType_Wheat] >= 5) return;

        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry) {
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Field, pos, m_target);
            }
        }

        if (m_hasTarget) {
            m_storage[ResourceType_Wheat]++;
        }
    }
};

} // namespace World

#endif
