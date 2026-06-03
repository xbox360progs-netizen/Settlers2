#ifndef WORLD_COMPONENTS_SAWMILL_H
#define WORLD_COMPONENTS_SAWMILL_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Sawmill : public Building {
public:
    Sawmill(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Sawmill, x, y, o, m) {
        inputResources.push_back(ResourceType_Wood);
        outputResources.push_back(ResourceType_Planks);
    }

    void Update() override {
        if (inventory[ResourceType_Wood] > 0 && inventory[ResourceType_Planks] < 5) {
            inventory[ResourceType_Wood]--;
            inventory[ResourceType_Planks]++;
        }
    }
};

} // namespace World

#endif
