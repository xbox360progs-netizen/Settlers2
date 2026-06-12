#ifndef WORLD_COMPONENTS_HUNTER_H
#define WORLD_COMPONENTS_HUNTER_H

#include "Building.h"
#include "../WildlifeSystem.h"
#include "../../Logic/ResourceRegistry.h"

namespace World {

class Hunter : public Building {
    int m_trapsCount;
public:
    Hunter(int x, int y, uint8_t o, Map* m)
        : Building(BuildingType::Hunter, x, y, o, m), m_trapsCount(0) {
        m_productionInterval = 5.0f;
        inputResources.push_back(ResourceType_Trap);
        outputResources.push_back(ResourceType_Meat);
    }

    void TrySetTraps() {
        if (m_trapsCount >= 3 || m_storage[ResourceType_Trap] <= 0) return;
        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry)
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_WildlifeSpawner_Deer, pos, m_target);
        }
        if (!m_hasTarget) return;
        WildlifeSystem* ws = map ? map->GetWildlifeSystem() : NULL;
        if (!ws) return;
        int animalIdx = ws->FindAliveAnimal(m_target.x, m_target.y, 4, AnimalType_Deer);
        if (animalIdx < 0) return;
        m_storage[ResourceType_Trap]--;
        ws->TrapAnimal(animalIdx);
        m_trapsCount++;
    }

    bool CanProduce() override {
        // Always try to find deer nearby for basic hunting
        if (!m_hasTarget) {
            Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
            if (registry)
                m_hasTarget = registry->FindNearestWorldResource(ResourceType_WildlifeSpawner_Deer, pos, m_target);
        }
        TrySetTraps();
        return (m_trapsCount > 0 || m_hasTarget) && !IsOutputFull();
    }

    bool ProduceOne() override {
        if (m_trapsCount > 0) {
            m_trapsCount--;
            return AddOutput(ResourceType_Meat, 1);
        }
        if (m_hasTarget) {
            return AddOutput(ResourceType_Meat, 1);
        }
        return false;
    }
};

} // namespace World

#endif
