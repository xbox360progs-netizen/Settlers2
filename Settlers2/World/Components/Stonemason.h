#ifndef WORLD_COMPONENTS_STONEMASON_H
#define WORLD_COMPONENTS_STONEMASON_H

#include "Building.h"
#include "../Map.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Stonemason : public Building {
public:
    Stonemason(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Stonemason, x, y, o, m) {
        m_productionInterval = 5.0f;
        outputResources.push_back(ResourceType_Stone);
    }

    bool CanProduce() override {
        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry)
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Granite, pos, m_target);
        }
        return m_hasTarget && !IsOutputFull();
    }

    bool ProduceOne() override {
        return AddOutput(ResourceType_Stone, 1);
    }
};

} // namespace World

#endif
