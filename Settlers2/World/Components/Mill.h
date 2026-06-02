#ifndef WORLD_COMPONENTS_MILL_H
#define WORLD_COMPONENTS_MILL_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Mill : public Building {
public:
    Mill(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Mill, x, y, o, m) {
        inputResources.push_back(ResourceType_Wheat); 
        outputResources.push_back(ResourceType_Flour);
    }

    void Update() override {
        // Логика мельницы: если есть пшеница, перемалываем в муку
        if (inventory[ResourceType_Wheat] > 0) {
            inventory[ResourceType_Wheat]--; // Потребляем пшеницу
            inventory[ResourceType_Flour]++;  // Производим муку
        }
    }
};

} // namespace World


#endif
