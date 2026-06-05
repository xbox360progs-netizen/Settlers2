#ifndef WORLD_COMPONENTS_STONEMASON_H
#define WORLD_COMPONENTS_STONEMASON_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Stonemason : public Building {
public:
    Stonemason(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Stonemason, x, y, o, m) {
        outputResources.push_back(ResourceType_Stone);
    }

    void Update() override {
        if (m_storage[ResourceType_Stone] >= 5) return;

        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry) {
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Granite, pos, m_target);
            }
        }

        if (m_hasTarget) {
            m_storage[ResourceType_Stone]++;
        }
    }
};

} // namespace World

#endif
