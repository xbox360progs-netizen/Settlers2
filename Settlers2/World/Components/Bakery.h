#ifndef WORLD_COMPONENTS_BAKERY_H
#define WORLD_COMPONENTS_BAKERY_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Bakery : public Building {
public:
    Bakery(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Bakery, x, y, o, m) {
        inputResources.push_back(ResourceType_Flour); 
        inputResources.push_back(ResourceType_Water);
        outputResources.push_back(ResourceType_Bread); 
    }

    void Update() override {
        // Логика пекарни: если есть мука и вода, печем хлеб
        if (inventory[ResourceType_Flour] > 0 && inventory[ResourceType_Water] > 0) {
            inventory[ResourceType_Flour]--;
            inventory[ResourceType_Water]--;
            inventory[ResourceType_Bread]++; // Хлеб
        }
    }
};

} // namespace World


#endif
