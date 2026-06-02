#ifndef WORLD_COMPONENTS_TOOLWORKSHOP_H
#define WORLD_COMPONENTS_TOOLWORKSHOP_H

#include "Building.h"
#include "../Map.h"

namespace World {

class ToolWorkshop : public Building {
public:
    ToolWorkshop(int x, int y, uint8_t o, Map* m) 
        : Building(BuildingType::ToolWorkshop, x, y, o, m) {
        inputResources.push_back(ResourceType_Wood);
        inputResources.push_back(ResourceType_Iron);
        outputResources.push_back(ResourceType_Trap);
    }

    void Update() override {
        // Логика мастерской: потребляет дерево/железо, производит инструменты/капканы
        if (inventory[ResourceType_Wood] > 0 && inventory[ResourceType_Iron] > 0) {
            inventory[ResourceType_Wood]--;
            inventory[ResourceType_Iron]--;
            inventory[ResourceType_Trap]++;
        }
    }
};

} // namespace World

#endif
