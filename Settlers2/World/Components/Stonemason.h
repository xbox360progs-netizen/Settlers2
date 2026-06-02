#ifndef WORLD_COMPONENTS_STONEMASON_H
#define WORLD_COMPONENTS_STONEMASON_H

#include "Building.h"

namespace World {

class Stonemason : public Building {
public:
    Stonemason(int x, int y, uint8_t o) 
        : Building(BuildingType::Stonemason, x, y, o) {
        // Stonemason needs access to granite deposits
        outputResources.push_back(ResourceType_Stone);
    }

    void Update() override {
        // Логика каменотеса: добыча камня из залежей гранита
    }
};

} // namespace World

#endif
