#ifndef WORLD_COMPONENTS_HUNTER_H
#define WORLD_COMPONENTS_HUNTER_H

#include "Building.h"
#include "../WildlifeSystem.h"

namespace World {

class Hunter : public Building {
public:
    int trapsCount;

    Hunter(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Hunter, x, y, o, m), trapsCount(0) {
        outputResources.push_back(ResourceType_Meat);
    }

    void Update() {
        // 1. Place traps on live animals
        if (trapsCount < 3 && inventory[ResourceType_Trap] > 0) {
            WildlifeSystem* ws = map ? map->GetWildlifeSystem() : NULL;
            if (ws) {
                int animalIdx = ws->FindAliveAnimal(pos.x, pos.y, 8, AnimalType_Deer);
                if (animalIdx >= 0) {
                    inventory[ResourceType_Trap]--;
                    ws->TrapAnimal(animalIdx);
                    trapsCount++;
                }
            }
        }

        // 2. Harvest meat from traps
        if (trapsCount > 0 && inventory[ResourceType_Meat] < 5) {
            inventory[ResourceType_Meat]++;
            trapsCount--;
        }
    }
};

} // namespace World

#endif
