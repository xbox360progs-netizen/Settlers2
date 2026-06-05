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
        outputResources.push_back(ResourceType_Meat);
    }

    void Update() {
        if (m_trapsCount < 3 && m_storage[ResourceType_Trap] > 0) {
            if (!m_hasTarget) {
                Logic::ResourceRegistry* registry = map ? map->GetResourceRegistry() : NULL;
                if (registry) {
                    m_hasTarget = registry->FindNearestWorldResource(ResourceType_WildlifeSpawner_Deer, pos, m_target);
                }
            }

            if (m_hasTarget) {
                WildlifeSystem* ws = map ? map->GetWildlifeSystem() : NULL;
                if (ws) {
                    int animalIdx = ws->FindAliveAnimal(m_target.x, m_target.y, 4, AnimalType_Deer);
                    if (animalIdx >= 0) {
                        m_storage[ResourceType_Trap]--;
                        ws->TrapAnimal(animalIdx);
                        m_trapsCount++;
                    }
                }
            }
        }

        if (m_trapsCount > 0 && m_storage[ResourceType_Meat] < 5) {
            m_storage[ResourceType_Meat]++;
            m_trapsCount--;
        }
    }
};

} // namespace World

#endif
