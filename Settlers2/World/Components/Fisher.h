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
        m_productionInterval = 4.0f;
        outputResources.push_back(ResourceType_Fish);
    }

    bool CanProduce() override {
        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry)
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Fish, pos, m_target);
        }
        return m_hasTarget && !IsOutputFull();
    }

    bool ProduceOne() override {
        return AddOutput(ResourceType_Fish, 1);
    }
};

} // namespace World

#endif
