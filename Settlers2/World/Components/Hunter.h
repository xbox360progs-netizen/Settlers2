#ifndef WORLD_COMPONENTS_HUNTER_H
#define WORLD_COMPONENTS_HUNTER_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Hunter : public Building {
public:
    int trapsCount;

    Hunter(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Hunter, x, y, o, m), trapsCount(0) {
        outputResources.push_back(ResourceType_Meat);
    }

    void Update() override {
        // 1. Ставим капканы (потребляем капканы из инвентаря)
        if (trapsCount < 3 && inventory[ResourceType_Trap] > 0) {
            int foundX, foundY;
            if (map && map->FindResourceInRadius(pos.x, pos.y, 8, ResourceType_WildlifeSpawner_Deer, foundX, foundY)) {
                inventory[ResourceType_Trap]--;
                trapsCount++;
            }
        }

        // 2. Охотимся
        if (trapsCount > 0) {
            if (inventory[ResourceType_Meat] < 5) {
                inventory[ResourceType_Meat]++;
            }
        }
    }
};

} // namespace World

#endif
