#ifndef WORLD_COMPONENTS_WOODCUTTER_H
#define WORLD_COMPONENTS_WOODCUTTER_H

#include "../Map.h"
#include "Building.h"

namespace World {

class Woodcutter : public Building {
public:
    Woodcutter(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::Woodcutter, x, y, o, m) {
        outputResources.push_back(ResourceType_Wood);
    }

    void Update() override {
        // Логика: ищем дерево (ResourceType_Wood) в радиусе 5
        if (inventory[ResourceType_Wood] < 5) {
            int foundX, foundY;
            if (map && map->FindResourceInRadius(pos.x, pos.y, 5, ResourceType_Wood, foundX, foundY)) {
                // Если дерево найдено, "рубим" (имитация)
                inventory[ResourceType_Wood]++;
            }
        }
    }

    }

} // namespace World

#endif
