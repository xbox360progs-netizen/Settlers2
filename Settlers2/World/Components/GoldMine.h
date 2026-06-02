#ifndef WORLD_COMPONENTS_GOLDMINE_H
#define WORLD_COMPONENTS_GOLDMINE_H

#include "Building.h"
#include "../Map.h"

namespace World {

class GoldMine : public Building {
public:
    GoldMine(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::GoldMine, x, y, o, m) {
        outputResources.push_back(ResourceType_Gold);
    }

    void Update() override {
        int foundX, foundY;
        if (map && map->FindTileTypeInRadius(pos.x, pos.y, 2, Objects, Mountain, foundX, foundY)) {
            int foodBonus = ConsumeFood();
            if (foodBonus > 0) {
                inventory[ResourceType_GoldOre] += foodBonus;
            }
        }
    }
};

} // namespace World

#endif
