#ifndef WORLD_COMPONENTS_WOODCUTTER_H
#define WORLD_COMPONENTS_WOODCUTTER_H

#include "../Map.h"
#include "Building.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Woodcutter : public Building {
public:
    Woodcutter(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Woodcutter, x, y, o, m) {
        m_productionInterval = 3.0f;
        outputResources.push_back(ResourceType_Wood);
    }


    bool CanProduce() override {
        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry)
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_Wood, pos, m_target);
        }
        return m_hasTarget && !IsOutputFull();
    }

    bool ProduceOne() override {
        return AddOutput(ResourceType_Wood, 1);
    }
};

} // namespace World

#endif
