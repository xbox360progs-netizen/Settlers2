#ifndef WORLD_COMPONENTS_FISHER_H
#define WORLD_COMPONENTS_FISHER_H

#include "Building.h"
#include "../Map.h"

namespace World {

class Fisher : public Building {
public:
    Fisher(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Fisher, x, y, o, m) {
        inputResources.push_back(ResourceType_Meat); // Используем Meat как Bread (Хлеб) для прикормки
        outputResources.push_back(ResourceType_Fish);
    }

    void Update() override {
        // Логика рыбака:
        // 1. Нужно иметь прикормку (хлеб)
        if (inventory[ResourceType_Meat] > 0) {
            // 2. Ищем воду в радиусе
            int foundX, foundY;
            if (map && map->FindResourceInRadius(pos.x, pos.y, 5, ResourceType_Fish, foundX, foundY)) {
                // 3. Используем прикормку и ловим рыбу
                inventory[ResourceType_Meat]--;
                if (inventory[ResourceType_Fish] < 5) {
                    inventory[ResourceType_Fish]++;
                }
            }
        }
    }
};

} // namespace World

#endif
