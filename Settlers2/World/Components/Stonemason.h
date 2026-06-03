#ifndef WORLD_COMPONENTS_STONEMASON_H
#define WORLD_COMPONENTS_STONEMASON_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Stonemason : public Building {
public:
    Stonemason(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Stonemason, x, y, o, m) {
        outputResources.push_back(ResourceType_Stone);
    }

    void Update() override {
        if (inventory[ResourceType_Stone] < 5) {
            int foundX, foundY;
            if (map && map->FindResourceInRadius(pos.x, pos.y, 5, ResourceType_Granite, foundX, foundY)) {
                inventory[ResourceType_Stone]++;
            }
        }
    }
};

} // namespace World

#endif
