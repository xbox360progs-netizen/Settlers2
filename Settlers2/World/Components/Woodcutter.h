#ifndef WORLD_COMPONENTS_WOODCUTTER_H
#define WORLD_COMPONENTS_WOODCUTTER_H

#include "../Map.h"
#include "Building.h"

namespace World {

class Woodcutter : public Building {
public:
    Woodcutter(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Woodcutter, x, y, o, m) {
        outputResources.push_back(ResourceType_Wood);
    }


    void Update() override {
        if (m_storage[ResourceType_Wood] >= 5) return;

        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry) {
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Wood, pos, m_target);
            }
        }

        if (m_hasTarget) {
            m_storage[ResourceType_Wood]++;
        }
    }
};

} // namespace World

#endif
