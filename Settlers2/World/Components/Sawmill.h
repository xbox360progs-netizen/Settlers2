#ifndef WORLD_COMPONENTS_SAWMILL_H
#define WORLD_COMPONENTS_SAWMILL_H

#include "Building.h"

namespace World {

class Sawmill : public Building {
public:
    Sawmill(int x, int y, uint8_t o) 
        : Building(BuildingType::Sawmill, x, y, o) {
        inputResources.push_back(ResourceType_Wood);
        outputResources.push_back(ResourceType_Wood); // Boards
    }

    void Update() override {
        // Логика пилорамы: получение древесины -> производство досок
    }
};

} // namespace World

#endif
