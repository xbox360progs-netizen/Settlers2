#ifndef WORLD_COMPONENTS_COALMINE_H
#define WORLD_COMPONENTS_COALMINE_H

#include "Building.h"
#include "../Map.h"

namespace World {

class CoalMine : public Building {
public:
    CoalMine(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::CoalMine, x, y, o, m) {
        outputResources.push_back(ResourceType_Coal);
    }

    void Update() override {
        // Логика угольной шахты: ищем гору в радиусе + потребляем еду
        int foundX, foundY;
        if (map && map->FindTileTypeInRadius(pos.x, pos.y, 2, Objects, Mountain, foundX, foundY)) {
            int foodBonus = ConsumeFood();
            if (foodBonus > 0) {
                // Если есть еда, производим уголь. Бонус разнообразия ускоряет добычу
                inventory[ResourceType_Coal] += foodBonus;
            }
        }
    }
};

} // namespace World

#endif
