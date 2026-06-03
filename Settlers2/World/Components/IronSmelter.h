#ifndef WORLD_COMPONENTS_IRONSMELTER_H
#define WORLD_COMPONENTS_IRONSMELTER_H

#include "Building.h"
#include "../Map.h"

namespace World {

class IronSmelter : public Building {
public:
    IronSmelter(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::IronSmelter, x, y, o, m) {
        inputResources.push_back(ResourceType_IronOre);
        inputResources.push_back(ResourceType_Coal);
        outputResources.push_back(ResourceType_IronBar);
    }

    void Update() override {
        // Плавильня: IronOre + Coal -> IronBar
        if (inventory[ResourceType_IronOre] > 0 && inventory[ResourceType_Coal] > 0) {
            inventory[ResourceType_IronOre]--;
            inventory[ResourceType_Coal]--;
            inventory[ResourceType_IronBar]++;
        }
    }
};

} // namespace World

#endif
