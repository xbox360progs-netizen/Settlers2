#ifndef WORLD_COMPONENTS_GOLDSMELTER_H
#define WORLD_COMPONENTS_GOLDSMELTER_H

#include "Building.h"
#include "../Map.h"

namespace World {

class GoldSmelter : public Building {
public:
    GoldSmelter(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::GoldSmelter, x, y, o, m) {
        inputResources.push_back(ResourceType_GoldOre);
        inputResources.push_back(ResourceType_Coal);
        outputResources.push_back(ResourceType_GoldBar);
    }

    void Update() override {
        // Плавильня: GoldOre + Coal -> GoldBar
        if (inventory[ResourceType_GoldOre] > 0 && inventory[ResourceType_Coal] > 0) {
            inventory[ResourceType_GoldOre]--;
            inventory[ResourceType_Coal]--;
            inventory[ResourceType_GoldBar]++;
        }
    }
};

} // namespace World

#endif
