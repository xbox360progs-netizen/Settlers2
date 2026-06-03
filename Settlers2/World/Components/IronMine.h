#ifndef WORLD_COMPONENTS_IRONMINE_H
#define WORLD_COMPONENTS_IRONMINE_H

#include "Building.h"
#include "../Map.h"

namespace World {

class IronMine : public Building {
public:
    IronMine(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::IronMine, x, y, o, m) {
        outputResources.push_back(ResourceType_IronOre);
    }

    void Update() override {
        int foundX, foundY;
        if (map && map->FindTileTypeInRadius(pos.x, pos.y, 2, Objects, Mountain, foundX, foundY)) {
            int foodBonus = ConsumeFood();
            if (foodBonus > 0) {
                inventory[ResourceType_IronOre] += foodBonus;
            }
        }
    }
};

} // namespace World

#endif
