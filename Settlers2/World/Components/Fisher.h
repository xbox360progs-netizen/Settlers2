#ifndef WORLD_COMPONENTS_FISHER_H
#define WORLD_COMPONENTS_FISHER_H

#include "Building.h"
#include "../Map.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Fisher : public Building {
public:
    Fisher(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Fisher, x, y, o, m) {
        inputResources.push_back(ResourceType_Meat);
        outputResources.push_back(ResourceType_Fish);
    }

    void Update() override {
        if (m_storage[ResourceType_Fish] >= 5) return;
        if (m_storage[ResourceType_Meat] <= 0) return;

        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry) {
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Fish, pos, m_target);
            }
        }

        if (m_hasTarget) {
            m_storage[ResourceType_Meat]--;
            m_storage[ResourceType_Fish]++;
        }
    }
};

} // namespace World

#endif
