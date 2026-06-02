#ifndef WORLD_COMPONENTS_FARM_H
#define WORLD_COMPONENTS_FARM_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Farm : public Building {
public:
    Farm(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Farm, x, y, o, m) {
        outputResources.push_back(ResourceType_Wheat);
    }

    void Update() override {
        // Логика фермы: ищем поле в радиусе
        int foundX, foundY;
        if (map && map->FindResourceInRadius(pos.x, pos.y, 5, ResourceType_Field, foundX, foundY)) {
            if (inventory[ResourceType_Wheat] < 5) {
                inventory[ResourceType_Wheat]++;
            }
        }
    }
};

} // namespace World

#endif
